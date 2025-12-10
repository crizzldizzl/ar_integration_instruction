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

	/*
	 * Button classes to spawn --> to avoid premature loading and crashes
	 */
	UPROPERTY()
	TSubclassOf<AUxtPressableButtonActor> robot_button_class;
	UPROPERTY()
	TSubclassOf<AUxtPressableButtonActor> human_button_class;
	UPROPERTY()
	TSubclassOf<AUxtPressableButtonActor> unassign_button_class;

	/*
	 * Button instances --> spawned button actors
	 */
	UPROPERTY()
	AUxtPressableButtonActor* robot_button_instance = nullptr;
	UPROPERTY()
	AUxtPressableButtonActor* human_button_instance = nullptr;
	UPROPERTY()
	AUxtPressableButtonActor* unassign_button_instance = nullptr;

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

	/*
	 * Spawns a button of the given class at the given relative location
	 */
	void spawn_button
	(
		TSubclassOf<AUxtPressableButtonActor> button_class,
		AUxtPressableButtonActor*& out_instance,
		const FVector& relative_location
	);

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
	 * Follow Component for easy menu reachability and visibility
	 */
	UPROPERTY()
	UUxtFollowComponent* follow_ = nullptr;

	/** 
	 * Cached pointer to the assignment game state 
	 */
	TWeakObjectPtr<class A_integration_game_state> cached_game_state_;

};
