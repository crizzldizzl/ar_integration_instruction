// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"

#include "grpc_wrapper.h"

#include "procedural_mesh_actor.generated.h"

/**
 * @struct F_procedural_mesh_data
 *
 * struct containing data for a 
 * procedural mesh
 *
 * @var vertices positions of vertices
 * @var triangles indices corresponding to @ref{vertices}
 *		for mesh faces
 * @var normals vertex normals vertex[n] <-> normal[n]
 * @var mean_color color for all vertices
 */
USTRUCT(BlueprintType)
struct AR_INTEGRATION_API F_procedural_mesh_data
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<FVector> vertices;

	UPROPERTY()
	TArray<int32> triangles;

	UPROPERTY()
	TArray<FVector> normals;

	UPROPERTY()
	FLinearColor mean_color = FLinearColor::Black;
};

/**
 * @class A_procedural_mesh_actor
 *
 * class for an actor with runtime mesh either constructed from @ref{F_procedural_mesh_data}
 * for concrete meshes or cube wireframes with color
 */
UCLASS()
class AR_INTEGRATION_API A_procedural_mesh_actor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	A_procedural_mesh_actor();

	/**
	 * generates the mesh from data and sets the procedural mesh 
	 */
	UFUNCTION(BlueprintCallable)
	void set_from_data(const F_procedural_mesh_data& data);

	/**
	 * generates the cube mesh with a color and sets the procedural mesh
	 */
	UFUNCTION(BlueprintCallable)
	void wireframe(const FLinearColor& color);

	/**
	 * 
	 */
	UFUNCTION(BlueprintCallable)
	void set_selected(bool mesh_selected);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void PostLoad() override;
	virtual void PostActorCreated() override;

public:

	/*
	 * @var mesh generated mesh
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UProceduralMeshComponent* mesh;

private:

	/*
	 * @var global_opaque global material for meshes with data
	 */
	UPROPERTY()
	UMaterial* global_opaque;

	/*
	 * @var global_wire global wireframe material for cube mesh
	 */
	UPROPERTY()
	UMaterial* global_wire;

	/*
	 * @var opaque_material instanced global_opaque material
	 */
	UPROPERTY()
	UMaterialInstanceDynamic* opaque_material;

	/*
	 * @var wireframe_material instanced global_wire material
	 */
	UPROPERTY()
	UMaterialInstanceDynamic* wireframe_material;
};
