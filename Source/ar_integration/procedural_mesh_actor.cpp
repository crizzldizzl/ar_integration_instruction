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
	
	global_opaque_ = material_opaque.Object;
	global_wire_ = material_wire.Object;

	/**
	 * create instances of global materials for mesh
	 */
	opaque_material_ = UMaterialInstanceDynamic::Create(global_opaque_, mesh);
	wireframe_material_ = UMaterialInstanceDynamic::Create(global_wire_, mesh);
	
	RootComponent = mesh;
	mesh->bUseAsyncCooking = true;

	static ConstructorHelpers::FClassFinder<A_assignment_menu_actor> assignment_menu_bp
	(
		TEXT("Blueprint'/Game/assignment_menu.assignment_menu_C'")
	);

	if (assignment_menu_bp.Succeeded())
	{
		assignment_menu_class_ = assignment_menu_bp.Class;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Could not load the assignment menu blueprint!"));
	}

	active_menu_ = nullptr;
}

// Called when the game starts or when spawned
void A_procedural_mesh_actor::BeginPlay()
{
	Super::BeginPlay();
	
	if (auto* grab_target = FindComponentByClass<UUxtGrabTargetComponent>())
	{
		grab_target->OnBeginGrab.AddDynamic(this, &A_procedural_mesh_actor::handle_begin_grab);
		grab_target->OnEndGrab.AddDynamic(this, &A_procedural_mesh_actor::handle_end_grab);
	}
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
	
	mesh->SetMaterial(0, opaque_material_);
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
}

void A_procedural_mesh_actor::handle_begin_grab(UUxtGrabTargetComponent* grab_target, FUxtGrabPointerData pointer_data)
{
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

/** 
* Called by menu right before it destroys itself 
*/
void A_procedural_mesh_actor::on_assignment_menu_closed()
{
	active_menu_ = nullptr;
}