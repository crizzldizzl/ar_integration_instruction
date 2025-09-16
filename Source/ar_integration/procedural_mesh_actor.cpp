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
	
	mesh->SetMaterial(0, wireframe_material);
}