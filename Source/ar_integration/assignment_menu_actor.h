#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ChildActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

#include "Behaviors/UxtFollowComponent.h"
#include "Controls/UxtPressableButtonActor.h"
#include "Controls/UxtPressableButtonComponent.h"
#include "Controls/UxtBackplateComponent.h"

#include "selection_client.h"

#include "assignment_menu_actor.generated.h"

class A_procedural_mesh_actor;
class A_integration_game_state;
class UUxtPointerComponent;

/**
* Floating 3-button menu for assigning a grabbed block.
* Spawns when the user grabs the block; the free hand presses a button.
*/
UCLASS()
class AR_INTEGRATION_API A_assignment_menu_actor : public AActor
{
	GENERATED_BODY()

public:

	A_assignment_menu_actor();

	/** 
	* Called immediately after spawn to wire the block were controlling
	*/
	void initialise(A_procedural_mesh_actor* in_parent);

	/**
	* Programmatic close (e.g. parent releases grab without choosing)
	*/
	void close_menu();

	// ------------------------------ public components ------------------------------

	/*
	 * back plate static mesh component --> visual background for the buttons
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UUxtBackPlateComponent* back_plate;

	/**
	 * button actors (spawned as child actors, for hierarchical organization)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UChildActorComponent* robot_button_component;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UChildActorComponent* human_button_component;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UChildActorComponent* unassign_button_component;

protected:

	virtual void BeginPlay() override;

	/** 
	 * Button callbacks 
	 */
	UFUNCTION()
	void on_robot_pressed(UUxtPressableButtonComponent* button, UUxtPointerComponent* pointer);
	UFUNCTION()
	void on_human_pressed(UUxtPressableButtonComponent* button, UUxtPointerComponent* pointer);
	UFUNCTION()
	void on_unassign_pressed(UUxtPressableButtonComponent* button, UUxtPointerComponent* pointer);

	/**
	 * Handles assignment logic
	 */
	void handle_assignment(assignment_type assignment);

	/**
	 * Applies follow orientation every frame
	 */
	virtual void Tick(float DeltaSeconds) override;

private:

	// ------------------------------ private components ------------------------------

	/*
	 * Root component --> for positioning the menu in space
	 */
	UPROPERTY()
	USceneComponent* root_;

	/** 
	 * Cached pointer to the block that spawned us 
	 */
	UPROPERTY()
	TWeakObjectPtr<A_procedural_mesh_actor> parent_block_;

	/** 
	 * Cached pointer to the assignment game state 
	 */
	TWeakObjectPtr<class A_integration_game_state> cached_game_state_;

	/*
	 * to increase distance/decrease from camera when maintaining offset
	 */
	constexpr static float camera_offset_multiplier_ = 1.75f;

	/** 
	 * Whether there is a camera-relative offset to maintain 
	 */
	bool has_camera_relative_offset_ = false;

	/** 
	 * Camera-relative offset to maintain 
	 */
	FVector camera_offset_local_ = FVector::ZeroVector;
};
