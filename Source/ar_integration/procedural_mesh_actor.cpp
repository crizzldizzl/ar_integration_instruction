// Fill out your copyright notice in the Description page of Project Settings.


#include "procedural_mesh_actor.h"

// Sets default values
A_procedural_mesh_actor::A_procedural_mesh_actor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	/**
	 * Generate mesh member and add to actor
	 */
	mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GeneratedMesh"));

	/**
	 * Load global materials with engine reference
	 */
	static ConstructorHelpers::FObjectFinder<UMaterial> material_opaque(
		TEXT("Material'/Game/proced_mat.proced_mat'"));

	static ConstructorHelpers::FObjectFinder<UMaterial> material_wire(
		TEXT("Material'/Game/wireframe.wireframe'"));
	
	global_opaque = material_opaque.Object;
	global_wire = material_wire.Object;

	/**
	 * create instances of global materials for mesh
	 */
	opaque_material = UMaterialInstanceDynamic::Create(global_opaque, mesh);
	wireframe_material = UMaterialInstanceDynamic::Create(global_wire, mesh);
	
	RootComponent = mesh;
	mesh->bUseAsyncCooking = true;
}

// Called when the game starts or when spawned
void A_procedural_mesh_actor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void A_procedural_mesh_actor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// This is called when actor is spawned (at runtime or when you drop it into the world in editor)
void A_procedural_mesh_actor::PostActorCreated()
{
	Super::PostActorCreated();
}

// This is called when actor is already in level and map is opened
void A_procedural_mesh_actor::PostLoad()
{
	Super::PostLoad();
}

void A_procedural_mesh_actor::set_from_data(const F_procedural_mesh_data& data)
{
	TArray<FLinearColor> vertex_colors;
	vertex_colors.Init(data.mean_color, data.vertices.Num());

	mesh->ClearAllMeshSections();
	mesh->CreateMeshSection_LinearColor(
		0, data.vertices, data.triangles, data.normals, 
		{}, vertex_colors, {}, false);
	
	mesh->SetMaterial(0, opaque_material);
}

void A_procedural_mesh_actor::wireframe(const FLinearColor& color)
{
	TArray<FVector> vertices;
	vertices.Reserve(8);
	vertices.Add(FVector(-1.f, 1.f, -1.f));
	vertices.Add(FVector(1.f, 1.f, -1.f));
	vertices.Add(FVector(1.f, -1.f, -1.f));
	vertices.Add(FVector(-1.f, -1.f, -1.f));
	vertices.Add(FVector(-1.f, 1.f, 1.f));
	vertices.Add(FVector(1.f, 1.f, 1.f));
	vertices.Add(FVector(1.f, -1.f, 1.f));
	vertices.Add(FVector(-1.f, -1.f, 1.f));

	TArray<int32> triangles = 
	{
		1, 3, 0,
		7, 5, 4,
		4, 1, 0,
		5, 2, 1,
		2, 7, 3,
		0, 7, 4,
		1, 2, 3,
		7, 6, 5,
		4, 5, 1,
		5, 6, 2,
		2, 6, 7,
		0, 3, 7
	};

	TArray<FVector> normals =
	{
		FVector(-UE_INV_SQRT_3, UE_INV_SQRT_3, -UE_INV_SQRT_3),
		FVector(UE_INV_SQRT_3, UE_INV_SQRT_3, -UE_INV_SQRT_3),
		FVector(UE_INV_SQRT_3, -UE_INV_SQRT_3, -UE_INV_SQRT_3),
		FVector(-UE_INV_SQRT_3, -UE_INV_SQRT_3, -UE_INV_SQRT_3),
		FVector(-UE_INV_SQRT_3, UE_INV_SQRT_3, UE_INV_SQRT_3),
		FVector(UE_INV_SQRT_3, UE_INV_SQRT_3, UE_INV_SQRT_3),
		FVector(UE_INV_SQRT_3, -UE_INV_SQRT_3, UE_INV_SQRT_3),
		FVector(-UE_INV_SQRT_3, -UE_INV_SQRT_3, UE_INV_SQRT_3)
	};
	
	TArray<FLinearColor> vertex_colors;
	vertex_colors.Init(color, vertices.Num());

	mesh->ClearAllMeshSections();
	mesh->CreateMeshSection_LinearColor(
		0, vertices, triangles, normals,
		{}, vertex_colors, {}, false);
	
	mesh->SetMaterial(0, wireframe_material_);

	update_assignment_labels();
}

void A_procedural_mesh_actor::handle_begin_grab(UUxtGrabTargetComponent* grab_target, FUxtGrabPointerData pointer_data)
{
	if (!selectable_)
	{
		return;
	}

	if (active_menu_ || !assignment_menu_class_) return;

	// determine which hand is grabbing
	EControllerHand grabbing_hand = EControllerHand::AnyHand;
	if (pointer_data.NearPointer)
		grabbing_hand = pointer_data.NearPointer->Hand;
	else if (pointer_data.FarPointer)
		grabbing_hand = pointer_data.FarPointer->Hand;

	EControllerHand free_hand = EControllerHand::AnyHand;
	if (grabbing_hand == EControllerHand::Left)
		free_hand = EControllerHand::Right;
	else if (grabbing_hand == EControllerHand::Right)
		free_hand = EControllerHand::Left;

	FQuat palm_rot;
	FVector palm_pos;
	float  palm_radius = 0.f;

	// try to get palm position of free hand
	bool has_free_hand = false;
	if (free_hand == EControllerHand::Left || free_hand == EControllerHand::Right)
	{
		has_free_hand = IUxtHandTracker::Get().GetJointState(free_hand, EHandKeypoint::Palm, palm_rot, palm_pos, palm_radius);
	}

	FVector spawn_location;
	FRotator spawn_rotation;

	if (has_free_hand)
	{
		const FVector camera_location = UGameplayStatics::GetPlayerCameraManager(this, 0)->GetCameraLocation();
		const FVector to_camera = (camera_location - palm_pos).GetSafeNormal();

		// offset toward the camera so the menu isnt inside the hand
		spawn_location = palm_pos + to_camera * 12.f; // --> cm
		spawn_rotation = to_camera.Rotation();
	}
	else
	{
		// old fallback behaviour if the other hand isnt tracked
		spawn_location = GetActorLocation() + GetActorUpVector() * 0.22f + GetActorForwardVector() * 0.15f;
		spawn_rotation = (UGameplayStatics::GetPlayerCameraManager(this, 0)->GetCameraLocation() - spawn_location).Rotation();
	}

	FActorSpawnParameters params;
	params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	params.Owner = this;

	// try to refresh scenario before showing the menu
	if (A_integration_game_state* game_state = GetWorld()->GetGameState<A_integration_game_state>())
	{
		if (!game_state->is_scenario_ready() && !game_state->refresh_scenario())
		{
			UE_LOG(LogTemp, Warning, TEXT("[procedural_mesh_actor] Scenario refresh failed; showing menu with current scenario %d"), static_cast<int32>(game_state->get_scenario_mode()));
		}

		if (game_state->get_scenario_mode() == scenario_type::BASELINE)
		{
			// Baseline --> no selection menu at all
			return;
		}
	}
	
	active_menu_ = GetWorld()->SpawnActor<A_assignment_menu_actor>(assignment_menu_class_, spawn_location, spawn_rotation, params);

	if (active_menu_)
	{
		active_menu_->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		active_menu_->initialise(this);
		active_menu_->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}

}

void A_procedural_mesh_actor::handle_end_grab(UUxtGrabTargetComponent* grab_target, FUxtGrabPointerData pointer_data)
{
	if (active_menu_)
	{
		active_menu_->close_menu();
		active_menu_ = nullptr;
	}
}

void A_procedural_mesh_actor::on_assignment_menu_closed()
{
	active_menu_ = nullptr;
}

void A_procedural_mesh_actor::set_assignment_state(assignment_type assignment)
{
	if (current_assignment_ == assignment) return;

	current_assignment_ = assignment;

	const bool show_label = (assignment != assignment_type::UNASSIGNED);

	FText label_text;
	if (assignment == assignment_type::ROBOT)
	{
		label_text = FText::FromString(TEXT("R"));
	}
	else if (assignment == assignment_type::HUMAN)
	{
		label_text = FText::FromString(TEXT("H"));
	}
	else
	{
		label_text = FText::GetEmpty();
	}
		

	FColor label_color;
	if (assignment == assignment_type::ROBOT)
	{
		label_color = FColor(255, 140, 0);
	}
	else if (assignment == assignment_type::HUMAN)
	{
		label_color = FColor(0, 128, 255);
	}
	else
	{
		label_color = FColor::Transparent;
	}

	for (UTextRenderComponent* text : assignment_labels_)
	{
		if (!text) continue;
		text->SetText(label_text);
		text->SetTextRenderColor(label_color);
		text->SetHiddenInGame(!show_label);
	}

	mesh->SetCustomDepthStencilValue(assignment_to_stencil(assignment));
	mesh->MarkRenderStateDirty();

}

uint8 A_procedural_mesh_actor::assignment_to_stencil(assignment_type assignment) const
{
	switch (assignment)
	{
	case assignment_type::HUMAN:
		return 1;

	case assignment_type::ROBOT:
		return 2;

	default:
		return 0;
	}
}

void A_procedural_mesh_actor::update_assignment_labels()
{
	if (!mesh || assignment_labels_.Num() == 0) return;

	const FBoxSphereBounds bounds = mesh->Bounds;
	const FVector extent = bounds.BoxExtent;

	// letters slightly off the surface
	const float offset = 0.5f;     

	// scale to the block size
	const float labelSize = FMath::Max(extent.X, extent.Y) * 1.4f;

	struct loc_rot
	{
		FVector  location;
		FRotator rotation;
	};

	loc_rot poses[4] = 
	{
		// +X
		{ FVector(extent.X + offset, 0.f, 0.f), FRotator(0.f,   0.f, 0.f) },
		// -X
		{ FVector(-extent.X - offset, 0.f, 0.f), FRotator(0.f, 180.f, 0.f) },
		// +Y
		{ FVector(0.f,  extent.Y + offset, 0.f), FRotator(0.f,  90.f, 0.f) },
		// -Y
		{ FVector(0.f, -extent.Y - offset, 0.f), FRotator(0.f, -90.f, 0.f) }   
	};

	for (int32 i = 0; i < assignment_labels_.Num() && i < UE_ARRAY_COUNT(poses); ++i)
	{
		UTextRenderComponent* label = assignment_labels_[i];
		label->SetRelativeLocation(poses[i].location);
		label->SetRelativeRotation(poses[i].rotation);
		label->SetHorizontalAlignment(EHTA_Center);
		label->SetVerticalAlignment(EVRTA_TextCenter);
		label->SetWorldSize(labelSize);
	}
}

void A_procedural_mesh_actor::set_selectable(bool enable)
{
	selectable_ = enable;

	if (!grab_target_)
	{
		return;
	}

	// mesh is selectable
	if (enable)
	{
		// enable collision for mesh
		if (mesh)
		{
			mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			mesh->SetCollisionResponseToAllChannels(ECR_Block);
		}

		// enable grab target for near and far interaction with one hand
		grab_target_->InteractionMode = static_cast<int32>(EUxtInteractionMode::Near | EUxtInteractionMode::Far);
		grab_target_->GrabModes = static_cast<int32>(EUxtGrabMode::OneHanded);
		grab_target_->Activate();
	}

	// mesh is not selectable
	else
	{
		// disable collision for mesh
		if (mesh)
		{
			mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		// disable grab target
		grab_target_->ForceEndGrab();
		grab_target_->InteractionMode = 0;
		grab_target_->GrabModes = 0;
		grab_target_->Deactivate();
	}
}

bool A_procedural_mesh_actor::is_selectable() const
{
	return selectable_;
}
