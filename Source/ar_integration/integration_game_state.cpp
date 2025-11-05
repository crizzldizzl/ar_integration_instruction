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

	channel_ = NewObject<U_grpc_channel>();

	pin_component_ = CreateDefaultSubobject<USceneComponent>("pin_component");
	correction_component_ = CreateDefaultSubobject<USceneComponent>("correction_component");

}

void A_integration_game_state::BeginPlay()
{
	Super::BeginPlay();

	pin_component_->RegisterComponent();

	//correction_component->RegisterComponent();
	correction_component_->AttachToComponent(pin_component_, FAttachmentTransformRules::KeepRelativeTransform);
	correction_component_->SetRelativeTransform(FTransform(FQuat{FRotator{0., 2., 0.}}, FVector(-0.4, 1.3, 0.), FVector::One()));
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
	
	franka_voxel->AttachToComponent(correction_component_, FAttachmentTransformRules::KeepRelativeTransform);
	franka_tcps->AttachToComponent(correction_component_, FAttachmentTransformRules::KeepRelativeTransform);
	franka->AttachToComponent(correction_component_, FAttachmentTransformRules::KeepRelativeTransform);

	franka_client->on_voxel_data.AddDynamic(this, &A_integration_game_state::handle_voxels);
	franka_client->on_visual_change.AddDynamic(franka_voxel, &I_franka_Interface::set_visibility);

	franka_tcp_client->on_tcp_data.AddDynamic(this, &A_integration_game_state::handle_tcps);
	franka_tcp_client->on_visual_change.AddDynamic(franka_tcps, &I_franka_Interface::set_visibility);

	franka_joint_sync_client->on_sync_joint_data.AddDynamic(this, &A_integration_game_state::handle_sync_joints);
	franka_joint_sync_client->on_visual_change.AddDynamic(franka_controller_, &I_franka_Interface::set_visibility);

	//franka_joint_client->on_joint_data.AddDynamic(this, &A_integration_game_state::handle_joints);

	on_post_actors.Broadcast();
}

void A_integration_game_state::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	init();
	
	std::unique_lock top_lock(anchor_mutex_);

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
		std::scoped_lock lock(actor_mutex_, delete_mutex_);
		Swap(set_list_, to_set);
		Swap(delete_list_, to_delete);
		Swap(pending_prototypes_, pending_proto);
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
	if (target == old_target_) return;

	old_target_ = target;
	synced_ = false;

	/*
	 * Set channel of clients prior
	 * to broadcasting to avoid order of
	 * execution conflicts
	 */
	I_Base_Client_Interface::Execute_set_channel(debug_client, channel_);
	I_Base_Client_Interface::Execute_set_channel(franka_client, channel_);
	I_Base_Client_Interface::Execute_set_channel(franka_tcp_client, channel_);
	//I_Base_Client_Interface::Execute_set_channel(franka_joint_client, channel_);
	I_Base_Client_Interface::Execute_set_channel(franka_joint_sync_client, channel_);
	I_Base_Client_Interface::Execute_set_channel(pcl_client, channel_);
	I_Base_Client_Interface::Execute_set_channel(selection_client, channel_);

	/**
	 * create new object_client and bind all the signals to
	 * respective update queues
	 */
	object_client = NewObject<U_object_client>();
	I_Base_Client_Interface::Execute_set_channel(object_client, channel_);

	object_client->on_object_instance_data.AddDynamic(this, &A_integration_game_state::set_object_instance_data);
	object_client->on_object_instance_colored_box.AddDynamic(this, &A_integration_game_state::set_object_instance_colored_box);
	object_client->on_object_delete.AddDynamic(this, &A_integration_game_state::delete_object);
	
	I_Base_Client_Interface::Execute_set_channel(mesh_client, channel_);

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
	set_list_.Empty();
	delete_list_.Empty();

	/*if (anchor_pin)
	{
		UARBlueprintLibrary::UnpinComponent(pin_component);
		UARBlueprintLibrary::RemovePin(anchor_pin);
		anchor_pin = nullptr;
	}*/

	I_Base_Client_Interface::Execute_set_channel(hand_tracking_client, channel_);

	/**
	 * resync if anchor is set
	 */
#if PLATFORM_HOLOLENS
	if (anchor_pin)
#endif
	{
		sync_and_subscribe();
	}
	
	on_channel_change.Broadcast(channel_);
}

void A_integration_game_state::spawn_obj_proto(const FString& name)
{
	const auto prototype = object_prototypes_.Find(name);
	if (!prototype) return;
	
	const auto mesh = meshes_.Find(prototype->mesh_name);
	if (!mesh) return;

	FActorSpawnParameters spawn_params;
	spawn_params.bNoFail = true;
	
	const FTransform spawn_transform( FQuat::Identity, FVector::ZeroVector, prototype->bounding_box.GetExtent());

	auto newActor = GetWorld()->SpawnActor<A_procedural_mesh_actor>(A_procedural_mesh_actor::StaticClass(), spawn_transform, spawn_params);
	
	newActor->set_from_data(create_proc_mesh_data(*prototype, *mesh));
}

void A_integration_game_state::set_object_instance_data(const F_object_instance_data& data)
{
	std::unique_lock lock(actor_mutex_);

	object_instances_.Add(data.id, data);
	
	F_object_instance temp(TInPlaceType<deduce_type<decltype(data)>::type>{}, data);

	/**
	 * check if prototype is cached
	 * if not add to pending prototypes
	 */
	if (!object_prototypes_.Find(data.data.prototype_name))
		pending_prototypes_.Add(data.data.prototype_name);
	
	set_list_.Add(std::move(temp));
}

void A_integration_game_state::set_object_instance_colored_box(const F_object_instance_colored_box& data)
{
	
	std::unique_lock lock(actor_mutex_);

	box_instances_.Add(data.id, data);

	F_object_instance temp(
		TInPlaceType<deduce_type<decltype(data)>::type>{}, data);
	set_list_.Add(std::move(temp));
}

void A_integration_game_state::delete_object(const FString& id)
{
	std::unique_lock lock(delete_mutex_);
	object_instances_.Remove(id);
	delete_list_.Add(id);
}

void A_integration_game_state::set_assignment_mode(assignment_type assignment)
{
	if (current_assignment_ == assignment)
	{
		return;
	}

	current_assignment_ = assignment;
	UE_LOG(LogTemp, Log, TEXT("[A_integration_game_state] Assignment mode switched to %d"), static_cast<int32>(assignment));
}

assignment_type A_integration_game_state::get_assignment_mode() const
{
	return current_assignment_;
}

void A_integration_game_state::select_mesh_by_actor(A_procedural_mesh_actor* actor)
{
	if (!actor) return;

	std::unique_lock lock(actor_mutex_);

	// Find id by actor
	FString selected_id;
	for (const auto& kv : actors)
	{
		if (kv.Value == actor)
		{
			selected_id = kv.Key;
			break;
		}
	}

	if (selected_id.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[A_integration_game_state] No ID found for selected actor!"));
		return;
	}

	// Find pn_id by id
	int32 selected_pn_id = -1;
	if (object_instances_.Contains(selected_id))
	{
		selected_pn_id = object_instances_[selected_id].pn_id;
	}
	else if (box_instances_.Contains(selected_id))
	{
		selected_pn_id = box_instances_[selected_id].pn_id;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[A_integration_game_state] No PN-ID found for %s"), *selected_id);
		return;
	}

	const int32 assignment_raw = static_cast<int32>(current_assignment_);
	UE_LOG(LogTemp, Log, TEXT("[A_integration_game_state] Selected actor %s (PN-ID: %d, Assignment: %d)"), *selected_id, selected_pn_id, assignment_raw);

	// Send selection to server
	if (selection_client)
	{
		bool success = selection_client->send_selection(selected_id, selected_pn_id, current_assignment_);
		if (!success)
		{
			UE_LOG(LogTemp, Warning, TEXT("[A_integration_game_state] Failed to send selection to server!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[A_integration_game_state] Selection client is null!"));
	}
}

void A_integration_game_state::update_anchor_transform(
	const FTransform& anchor_transform)
{
	std::unique_lock lock(anchor_mutex_);
	if (anchor_pin_)
	{
		/**
		 * delete anchor and unseat workspace component
		 */
		//UARBlueprintLibrary::UnpinComponent(pin_component);
		UARBlueprintLibrary::RemovePin(anchor_pin_);
		UARBlueprintLibrary::RemoveARPinFromLocalStore(pin_save_name_);
	}
	/**
	 * reseat pin_component with new anchor transform
	 * save new pin
	 */
	anchor_pin_ = UARBlueprintLibrary::PinComponent(pin_component_, anchor_transform);
	UARBlueprintLibrary::SaveARPinToLocalStore(pin_save_name_, anchor_pin_);

	sync_and_subscribe(true);
}

void A_integration_game_state::sync_and_subscribe(bool forced)
{
	if (!forced && synced_) return;

	synced_ = true;

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
	mesh_client->get_object_prototypes(pending_proto.Array(), object_prototypes_);

	TSet<FString> new_meshes;
	for (const auto& proto : pending_proto)
	{
		/**
		 * if object prototype is still not present ignore
		 */
		auto it_0 = object_prototypes_.Find(proto);
		if (!it_0) continue;

		/**
		 * add mesh to new_meshes if not already cached
		 */
		auto it_1 = meshes_.Find(it_0->mesh_name);
		if (!it_1) new_meshes.Add(it_0->mesh_name);
	}

	/**
	 * load meshes which weren't cached
	 */
	mesh_client->get_meshes(new_meshes.Array(), meshes_);
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

	const int32 pn_id = Visit
	(
		Overload
		{
			[](const F_object_instance_data& instance_data)
			{
				return instance_data.pn_id;
			},
			[](const F_object_instance_colored_box& instance_cb)
			{
				return instance_cb.pn_id;
			}
		},
		instance
	);

	/**
	 * Spawn and/or change
	 */
	const bool earlyOut = Visit
	(
		Overload
		{
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
		},
		instance
	);

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
	actor->set_selectable(pn_id > 0);
	f(actor);
	actor->AttachToComponent(correction_component_, FAttachmentTransformRules::KeepRelativeTransform);

	actor->SetActorRelativeTransform(trafo);
}

void A_integration_game_state::init()
{
	/**
	 * load potentially existing workspace anchor at startup
	 */
	if (starting_ && UARBlueprintLibrary::IsARPinLocalStoreReady())
	{
		starting_ = false;
		auto pins = UARBlueprintLibrary::LoadARPinsFromLocalStore();
		auto it = pins.Find(pin_save_name_);
		if (it)
		{
			anchor_pin_ = *it;
			UARBlueprintLibrary::PinComponentToARPin(pin_component_, anchor_pin_);
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

bool A_integration_game_state::get_prototype_and_mesh
(
	const FString& proto_id,
	const F_object_prototype*& proto, 
	const F_mesh_data*& mesh)
{
	proto = object_prototypes_.Find(proto_id);
	if (!proto) return false;

	mesh = meshes_.Find(proto->mesh_name);
	if (!mesh) return false;

	return true;
}

F_procedural_mesh_data A_integration_game_state::create_proc_mesh_data
(
	const F_object_prototype& proto,
	const F_mesh_data& mesh)
{
	return 
	{
		mesh.vertices,
		mesh.indices,
		mesh.normals,
		FLinearColor(proto.mean_color)
	};
}

A_procedural_mesh_actor* A_integration_game_state::spawn_mesh_actor(const FString& id)
{

	FActorSpawnParameters spawn_params;
	spawn_params.bNoFail = true;
	
	auto temp = GetWorld()->SpawnActor<A_procedural_mesh_actor>(A_procedural_mesh_actor::StaticClass(), spawn_params);

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