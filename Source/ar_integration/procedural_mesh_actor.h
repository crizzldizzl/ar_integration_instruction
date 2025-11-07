// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"

#include "Components/TextRenderComponent.h"
#include "Interactions/UxtGrabTargetComponent.h"
#include "HandTracking/IUxtHandTracker.h"
#include "Input/UxtNearPointerComponent.h"
#include "Input/UxtFarPointerComponent.h"
#include "Interactions/UxtInteractionMode.h"

#include "grpc_wrapper.h"
#include "assignment_menu_actor.h"

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
	/*
	 * Sets default values for this actor's properties
	 */
	A_procedural_mesh_actor();

	/*
	 * sets the assignment state and updates material stencil value
	 */
	void set_assignment_state(assignment_type assignment);

	/*
	 * updates the assignment labels based on current assignment state
	 */
	void update_assignment_labels();

	/*
	 * enables or disables the mesh as selectable by the assignment system
	 */
	void set_selectable(bool enable);

	/*
	 * returns whether the mesh is selectable by the assignment system
	 */
	bool is_selectable() const;

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
	 * called when assignment menu is closed to null active menu
	 */
	UFUNCTION(BlueprintCallable)
	void on_assignment_menu_closed();

	// --- Functions called by the engine every frame ---

	virtual void Tick(float DeltaTime) override;

	virtual void PostLoad() override;

	virtual void PostActorCreated() override;

	/*
	 * @var mesh generated mesh
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UProceduralMeshComponent* mesh;

protected:

	/*
	 * Called when the game starts or when spawned
	 */
	virtual void BeginPlay() override;

	/*
	 * Called when properties are changed in editor or actor is spawned
	 */
	virtual void OnConstruction(const FTransform& Transform) override;

private:

	/*
	 * grab event handlers for spawning assignment menu
	 */
	UFUNCTION()
	void handle_begin_grab(UUxtGrabTargetComponent* grab_target, FUxtGrabPointerData pointer_data);
	UFUNCTION()
	void handle_end_grab(UUxtGrabTargetComponent* grab_target, FUxtGrabPointerData pointer_data);

	/*
	 * converts assignment type to stencil value
	 */
	uint8 assignment_to_stencil(assignment_type assignment) const;
	
	/*
	 * @var global_opaque global material for meshes with data
	 */
	UPROPERTY()
	UMaterial* global_opaque_;

	/*
	 * @var global_wire global wireframe material for cube mesh
	 */
	UPROPERTY()
	UMaterial* global_wire_;

	/*
	 * @var opaque_material instanced global_opaque material
	 */
	UPROPERTY()
	UMaterialInstanceDynamic* opaque_material_;

	/*
	 * @var wireframe_material instanced global_wire material
	 */
	UPROPERTY()
	UMaterialInstanceDynamic* wireframe_material_;

	/*
	 * @var grab_target_ grab target component for handling grab events
	 */
	UPROPERTY()
	UUxtGrabTargetComponent* grab_target_;

	/*
	 * @var assignment_menu_class class of the assignment menu actor to spawn
	 */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<A_assignment_menu_actor> assignment_menu_class_;

	/*
	 * @var active_menu currently active assignment menu
	 */
	UPROPERTY()
	A_assignment_menu_actor* active_menu_;

	/*
	 * @var assignment_labels_ text render components for assignment labels
	 */
	UPROPERTY()
	TArray<UTextRenderComponent*> assignment_labels_;

	/*
	 * @var current_assignment_ current assignment state of the mesh
	 */
	UPROPERTY()
	assignment_type current_assignment_ = assignment_type::UNASSIGNED;

	/*
	 * @var selectable_ whether the mesh is selectable by the assignment system
	 */
	UPROPERTY()
	bool selectable_ = false;

	// ------------------------------ testing members ------------------------------

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Testing", meta = (AllowPrivateAccess = "true"))
	int32 test_pn_id_ = -1;

	UFUNCTION(BlueprintCallable, Category = "Testing", meta = (AllowPrivateAccess = "true"))
	void apply_test_pn_id()
	{
		set_selectable(test_pn_id_ >= 0);
	}

};
