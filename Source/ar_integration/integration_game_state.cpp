#include "integration_game_state.h"
//#include "HeadMountedDisplayFunctionLibrary.h"

template<typename ... Ts>
struct Overload : Ts ... {
	using Ts::operator() ...;
};
template<class... Ts> Overload(Ts...) -> Overload<Ts...>;

A_integration_game_state::A_integration_game_state()
{
	auto root = CreateDefaultSubobject<USceneComponent>("root");
	SetRootComponent(root);

	PrimaryActorTick.bCanEverTick = true;

	debug_client = NewObject<U_debug_client>();
	mesh_client = NewObject<U_mesh_client>();
	selection_client = NewObject<U_selection_client>();

	franka_client = NewObject<U_franka_client>();
	franka_tcp_client = NewObject<U_franka_tcp_client>();
	//franka_joint_client = NewObject<U_franka_joint_client>();
	franka_joint_sync_client = NewObject<U_franka_joint_sync_client>();

	franka_controller_ = NewObject<U_franka_shadow_controller>();

	channel = NewObject<U_grpc_channel>();

	pin_component = CreateDefaultSubobject<USceneComponent>("pin_component");
	correction_component = CreateDefaultSubobject<USceneComponent>("correction_component");

	static ConstructorHelpers::FObjectFinder<UBlueprint> BP_ProcMeshActor(
		TEXT("Blueprint'/Game/BP_ProceduralMeshActor.BP_ProceduralMeshActor'")
	);

	if (BP_ProcMeshActor.Succeeded())
	{
		procedural_mesh_BP_class = BP_ProcMeshActor.Object->GeneratedClass;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Could not load BP_ProceduralMeshActor!"));
	}
}

void A_integration_game_state::BeginPlay()
{
	Super::BeginPlay();
	pin_component->RegisterComponent();

	//correction_component->RegisterComponent();
	correction_component->AttachToComponent(pin_component, FAttachmentTransformRules::KeepRelativeTransform);
	correction_component->SetRelativeTransform(FTransform(FQuat{FRotator{0., 2., 0.}}, FVector(-0.4, 1.3, 0.), FVector::One()));
	/*pin_component->AttachToComponent(GetRootComponent(),
		FAttachmentTransformRules::KeepWorldTransform);*/

	FActorSpawnParameters params;
	params.bNoFail = true;

	pcl_client = GetWorld()->SpawnActor<A_pcl_client>(params);
	//pcl_client->visualize = false;

	hand_tracking_client = GetWorld()->SpawnActor<A_hand_tracking_client>(params);

	franka_voxel = GetWorld()->SpawnActor<A_franka_voxel>(params);
	franka_tcps = GetWorld()->SpawnActor<A_franka_tcps>(params);

	franka = GetWorld()->SpawnActor<AFranka>(params);
	franka_controller_->set_robot(franka);
	
	franka_voxel->AttachToComponent(correction_component,
		FAttachmentTransformRules::KeepRelativeTransform);
	franka_tcps->AttachToComponent(correction_component,
		FAttachmentTransformRules::KeepRelativeTransform);
	franka->AttachToComponent(correction_component,
		FAttachmentTransformRules::KeepRelativeTransform);

	franka_client->on_voxel_data.AddDynamic(this, &A_integration_game_state::handle_voxels);
	franka_client->on_visual_change.AddDynamic(franka_voxel, &I_franka_Interface::set_visibility);

	franka_tcp_client->on_tcp_data.AddDynamic(this, &A_integration_game_state::handle_tcps);
	franka_tcp_client->on_visual_change.AddDynamic(franka_tcps, &I_franka_Interface::set_visibility);

	franka_joint_sync_client->on_sync_joint_data.AddDynamic(this, &A_integration_game_state::handle_sync_joints);
	franka_joint_sync_client->on_visual_change.AddDynamic(franka_controller_, &I_franka_Interface::set_visibility);

	//franka_joint_client->on_joint_data.AddDynamic(this, &A_integration_game_state::handle_joints);

	on_post_actors.Broadcast();


	// --- Selection Test ---
	// for:
	// --> programmatic generation of selectable meshes
	// --> dummy for sending mesh id to server

	const FString TestId = TEXT("TestActor_1");
	A_procedural_mesh_actor* TestActor = spawn_mesh_actor(TestId);

	if (TestActor)
	{
		TestActor->SetActorLocation(FVector(200, 0, 50));
		TestActor->wireframe(FLinearColor::Yellow);

		UE_LOG(LogTemp, Log, TEXT("[IntegrationGameState] Spawned test mesh actor 'TestActor_1'"));
	}
}

void A_integration_game_state::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	init();
	
	std::unique_lock top_lock(anchor_mutex);

#if PLATFORM_HOLOLENS
	if (!anchor_pin) return;
#endif
	
	TArray<FString> to_delete;
	TArray<F_object_instance> to_set;
	TSet<FString> pending_proto;
	{
		/**
		 * swap update data to block threads for as short as possible
		 */
		std::scoped_lock lock(actor_mutex, delete_mutex);
		Swap(set_list, to_set);
		Swap(delete_list, to_delete);
		Swap(pending_prototypes, pending_proto);
	}

	update_meshes(pending_proto);
	update_actors(to_delete);	

	to_set = to_set.FilterByPredicate([&to_delete](const F_object_instance& instance)
		{
			return !to_delete.Contains(get_object_instance_id(instance));
		});

	for (const auto& set : to_set)
		handle_object_instance(set);

	franka_controller_->Tick(DeltaSeconds);
}

void A_integration_game_state::change_channel(FString target, int32 retries)
{
	if (target == old_target) return;
	
	if (!channel->construct(target, 400, retries)) return;

	old_target = target;
	synced = false;

	/*
	 * Set channel of clients prior
	 * to broadcasting to avoid order of
	 * execution conflicts
	 */
	I_Base_Client_Interface::Execute_set_channel(debug_client, channel);
	I_Base_Client_Interface::Execute_set_channel(franka_client, channel);
	I_Base_Client_Interface::Execute_set_channel(franka_tcp_client, channel);
	//I_Base_Client_Interface::Execute_set_channel(franka_joint_client, channel);
	I_Base_Client_Interface::Execute_set_channel(franka_joint_sync_client, channel);
	I_Base_Client_Interface::Execute_set_channel(pcl_client, channel);
	/**
	 * create new object_client and bind all the signals to
	 * respective update queues
	 */
	object_client = NewObject<U_object_client>();
	I_Base_Client_Interface::Execute_set_channel(object_client, channel);

	object_client->on_object_instance_data.AddDynamic(
		this, &A_integration_game_state::set_object_instance_data);
	object_client->on_object_instance_colored_box.AddDynamic(
		this, &A_integration_game_state::set_object_instance_colored_box);
	object_client->on_object_delete.AddDynamic(
		this, &A_integration_game_state::delete_object);
	
	I_Base_Client_Interface::Execute_set_channel(mesh_client, channel);

	I_Base_Client_Interface::Execute_set_channel(selection_client, channel);

	/**
	 * remove all the actors from the scene
	 */
	for (const auto& actor : actors)
	{
		if (actor.Value)
			actor.Value->Destroy();
	}
	/**
	 * delete all update lists
	 */
	actors.Empty();
	set_list.Empty();
	delete_list.Empty();

	/*if (anchor_pin)
	{
		UARBlueprintLibrary::UnpinComponent(pin_component);
		UARBlueprintLibrary::RemovePin(anchor_pin);
		anchor_pin = nullptr;
	}*/

	I_Base_Client_Interface::Execute_set_channel(hand_tracking_client, channel);

	/**
	 * resync if anchor is set
	 */
#if PLATFORM_HOLOLENS
	if (anchor_pin)
#endif
	{
		sync_and_subscribe();
	}
	
	on_channel_change.Broadcast(channel);
}

void A_integration_game_state::spawn_obj_proto(const FString& name)
{
	const auto prototype = object_prototypes.Find(name);
	if (!prototype) return;
	
	const auto mesh = meshes.Find(prototype->mesh_name);
	if (!mesh) return;

	FActorSpawnParameters spawnParams;
	spawnParams.bNoFail = true;
		
	const auto newActor = GetWorld()->SpawnActor<A_procedural_mesh_actor>(
		A_procedural_mesh_actor::StaticClass(), 
		FTransform(FQuat::Identity, FVector::ZeroVector, prototype->bounding_box.GetExtent()),
		spawnParams);
	
	newActor->set_from_data(create_proc_mesh_data(*prototype, *mesh));
}

void A_integration_game_state::set_object_instance_data(
	const F_object_instance_data& data)
{
	std::unique_lock lock(actor_mutex);
	
	F_object_instance temp(
		TInPlaceType<deduce_type<decltype(data)>::type>{}, data);

	/**
	 * check if prototype is cached
	 * if not add to pending prototypes
	 */
	if (!object_prototypes.Find(data.data.prototype_name)) 
		pending_prototypes.Add(data.data.prototype_name);
	
	set_list.Add(std::move(temp));
}

void A_integration_game_state::set_object_instance_colored_box(
	const F_object_instance_colored_box& data)
{
	std::unique_lock lock(actor_mutex);
	F_object_instance temp(
		TInPlaceType<deduce_type<decltype(data)>::type>{}, data);
	set_list.Add(std::move(temp));
}

void A_integration_game_state::delete_object(const FString& id)
{
	std::unique_lock lock(delete_mutex);
	delete_list.Add(id);
}

void A_integration_game_state::select_mesh_by_actor(A_procedural_mesh_actor* actor)
{
	if (!actor) return;

	// find id by actor
	FString selectedId;
	for (const auto& kv : actors)
	{
		if (kv.Value == actor)
		{
			selectedId = kv.Key;
			break;
		}
	}

	if (selectedId.IsEmpty()) return;

	UE_LOG(LogTemp, Log, TEXT("Selected actor with id: %s"), *selectedId);

	// send id to server
	if (selection_client)
	{
		selection_client->send_selection(selectedId);
	}
}

void A_integration_game_state::update_anchor_transform(
	const FTransform& anchor_transform)
{
	std::unique_lock lock(anchor_mutex);
	if (anchor_pin)
	{
		/**
		 * delete anchor and unseat workspace component
		 */
		//UARBlueprintLibrary::UnpinComponent(pin_component);
		UARBlueprintLibrary::RemovePin(anchor_pin);
		UARBlueprintLibrary::RemoveARPinFromLocalStore(pin_save_name);
	}
	/**
	 * reseat pin_component with new anchor transform
	 * save new pin
	 */
	anchor_pin = UARBlueprintLibrary::PinComponent(pin_component, anchor_transform);
	UARBlueprintLibrary::SaveARPinToLocalStore(pin_save_name, anchor_pin);

	sync_and_subscribe(true);
}

void A_integration_game_state::sync_and_subscribe(bool forced)
{
	if (!forced && synced) return;

	synced = true;

	if (object_client)
	{
		object_client->sync_objects();
		object_client->async_subscribe_objects();
		object_client->async_subscribe_delete_objects();
	}

#if PLATFORM_HOLOLENS
	hand_tracking_client->update_local_transform(anchor_pin->GetLocalToWorldTransform().Inverse());
#endif
	hand_tracking_client->async_transmit_data();

	franka_client->async_transmit_data();
	franka_tcp_client->async_transmit_data();
	//franka_joint_client->async_transmit_data();
	franka_joint_sync_client->async_transmit_data();
}

void A_integration_game_state::update_meshes(const TSet<FString>& pending_proto)
{
	/**
	 * try to get object prototypes from server and cache them
	 */
	mesh_client->get_object_prototypes(pending_proto.Array(),
		object_prototypes);

	TSet<FString> new_meshes;
	for (const auto& proto : pending_proto)
	{
		/**
		 * if object prototype is still not present ignore
		 */
		auto it_0 = object_prototypes.Find(proto);
		if (!it_0) continue;

		/**
		 * add mesh to new_meshes if not already cached
		 */
		auto it_1 = meshes.Find(it_0->mesh_name);
		if (!it_1) new_meshes.Add(it_0->mesh_name);
	}

	/**
	 * load meshes which weren't cached
	 */
	mesh_client->get_meshes(new_meshes.Array(), meshes);
}

void A_integration_game_state::update_actors(const TArray<FString>& to_delete)
{
	/**
	 * actors might habe been destroyed globaly outside this scope
	 */
	for (const auto& actor : actors)
	{
		if (!IsValid(actor.Value))
			actors.Remove(actor.Key);
	}

	/**
	 * remove any actors from the scene and @ref{actors}
	 * which have been deleted
	 */
	A_procedural_mesh_actor* temp_del;
	for (const auto& del : to_delete)
	{
		if (actors.RemoveAndCopyValue(del, temp_del))
			temp_del->Destroy();
	}
}

void A_integration_game_state::handle_object_instance(const F_object_instance& instance)
{
	FTransform trafo;
	std::function<void(A_procedural_mesh_actor* actor)> f;

	/**
	 * Spawn and/or change
	 */
	const bool earlyOut = Visit(Overload{
		[&](const F_object_instance_data& instance_data)
		{
			const auto& data = instance_data.data;

			const F_object_prototype* prototype;
			const F_mesh_data* mesh;

			/**
			 * check if the prototypes and its mesh are cached
			 */
			if (!get_prototype_and_mesh(data.prototype_name, prototype, mesh))
				return true;

			/**
			 * calculate the actor transform
			 */
			trafo = data.transform.GetScaled(prototype->bounding_box.GetExtent());

			/**
			 * bind actor post constructor function
			 */
			f = [this, data = create_proc_mesh_data(*prototype, *mesh)]
			(A_procedural_mesh_actor* actor)
				{
					actor->set_from_data(data);
				};
			return false;
		},
		[&](const F_object_instance_colored_box& instance_cb)
		{
			const auto& [box, color] = instance_cb.data;
			/**
			 * calculate the actor transform
			 */
			trafo = FTransform(
				box.rotation,
				box.axis_box.GetCenter(),
				box.axis_box.GetExtent());

			/**
			 * bind actor post constructor function
			 */
			f = [this, actor_color = FLinearColor(color)]
			(A_procedural_mesh_actor* actor)
				{
					actor->wireframe(actor_color);
				};

			return false;
		}
	}, instance);

	if (earlyOut)
		return;

	/**
	 * 1. spawn or find
	 * 2. post spawn or update
	 * 3. attach to anchor component
	 * 4. set transform in workspace
	 */
	const FString id = get_object_instance_id(instance);
	A_procedural_mesh_actor* actor = find_or_spawn(id);
	f(actor);
	actor->AttachToComponent(correction_component,
		FAttachmentTransformRules::KeepRelativeTransform);

	actor->SetActorRelativeTransform(trafo);
}

void A_integration_game_state::init()
{
	/**
	 * load potentially existing workspace anchor at startup
	 */
	if (starting && UARBlueprintLibrary::IsARPinLocalStoreReady())
	{
		starting = false;
		auto pins = UARBlueprintLibrary::LoadARPinsFromLocalStore();
		auto it = pins.Find(pin_save_name);
		if (it)
		{
			anchor_pin = *it;
			UARBlueprintLibrary::PinComponentToARPin(pin_component, anchor_pin);
		}
	}
}

FString A_integration_game_state::get_object_instance_id(const F_object_instance& data)
{
	return Visit(Overload{
		[](const F_object_instance_data& instance) { return instance.id; },
		[](const F_object_instance_colored_box& instance) { return instance.id; }
		}, data);
}

bool A_integration_game_state::get_prototype_and_mesh(
	const FString& proto_id,
	const F_object_prototype*& proto, 
	const F_mesh_data*& mesh)
{
	proto = object_prototypes.Find(proto_id);
	if (!proto) return false;

	mesh = meshes.Find(proto->mesh_name);
	if (!mesh) return false;

	return true;
}

F_procedural_mesh_data A_integration_game_state::create_proc_mesh_data(
	const F_object_prototype& proto,
	const F_mesh_data& mesh)
{
	return {
		mesh.vertices,
		mesh.indices,
		mesh.normals,
		FLinearColor(proto.mean_color)
	};
}

A_procedural_mesh_actor* A_integration_game_state::spawn_mesh_actor(const FString& id)
{
	if (!procedural_mesh_BP_class)
	{
		UE_LOG(LogTemp, Error, TEXT("ProceduralMeshBPClass is null!"));
		return nullptr;
	}

	FActorSpawnParameters spawnParams;
	spawnParams.bNoFail = true;
	
	auto temp = GetWorld()->SpawnActor<A_procedural_mesh_actor>(
		procedural_mesh_BP_class, spawnParams);

	return actors.Emplace(id, temp);
	
}

A_procedural_mesh_actor* A_integration_game_state::find_or_spawn(const FString& id)
{
	const auto it = actors.Find(id);
	if (it) return *it;
	
	const auto temp = spawn_mesh_actor(id);
	temp->GetRootComponent()->SetMobility(EComponentMobility::Movable);

	return temp;
}

void A_integration_game_state::handle_voxels(const F_voxel_data& data)
{
	//if (!franka_voxel->IsHidden())
		franka_voxel->set_voxels(data);
}

void A_integration_game_state::handle_tcps(const TArray<FVector>& data)
{
	//if (!franka_tcps->IsHidden())
		franka_tcps->set_tcps(data);
}

void A_integration_game_state::handle_joints(const FFrankaJoints& data)
{
	franka->SetJoints(data);
}

void A_integration_game_state::handle_sync_joints(const TArray<F_joints_synced>& data)
{
	//if (!franka->IsHidden())
		franka_controller_->set_visual_plan(data);
}