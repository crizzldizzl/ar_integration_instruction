#pragma once

#include "ARPin.h"
#include "ARBlueprintLibrary.h"
#include "GameFramework/GameStateBase.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Blueprint.h"
#include "Engine/TextRenderActor.h"
#include "Engine/PostProcessVolume.h"
#include "Components/TextRenderComponent.h"

#include "grpc_channel.h"
#include "debug_client.h"
#include "object_client.h"
#include "selection_client.h"
#include "mesh_client.h"
#include "pcl_client.h"
#include "franka_client.h"
#include "franka_voxel.h"
#include "franka_tcps.h"
#include "Franka.h"
#include "franka_shadow.h"
#include "hand_tracking_client.h"
#include "grpc_wrapper.h"

#include "procedural_mesh_actor.h"

#include "integration_game_state.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(F_channel_delegate, U_grpc_channel*, channel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(F_post_actors_delegate);

/**
 * @class A_integration_game_state
 * class holding all the global state information
 * of the application
 *
 * contains methods affecting global changes
 * e.g. actors in the workspace
 */
UCLASS()
class AR_INTEGRATION_API A_integration_game_state : public AGameStateBase
{
	GENERATED_BODY()
	
public:

	typedef TVariant<F_object_instance_data, F_object_instance_colored_box> F_object_instance;

	/**
	 * initializes clients and arpin
	 */
	A_integration_game_state();

	virtual void BeginPlay() override;

	/**
	 * loading spatial anchor at the beginning of the application if
	 * present
	 * main loop for spawning actors from to_spawn sets
	 */
	virtual void Tick(float DeltaSeconds) override;

	/**
	 * sets/updates channel of all underlying clients
	 * if connection can be established successful to target
	 *
	 * resets all actors
	 *
	 * automatically synchronizes and subscribes in case anchor is present
	 *
	 * @attend references to previously created clients may be invalidated
	 * @attend emits @ref{on_channel_change} after all internal client channels are set
	 */
	UFUNCTION(BlueprintCallable)
	void change_channel(const FString& target, int32 retries = 1);

	/**
	 * signal emitted on valid change of channel
	 */
	UPROPERTY(BlueprintAssignable)
	F_channel_delegate on_channel_change;

	/**
	 * spawns object prototype based on name
	 * if prototype and its mesh are present
	 */
	UFUNCTION(BlueprintCallable)
	void spawn_obj_proto(const FString& name);

	/**
	 * thread safe insertion into set_list
	 */
	UFUNCTION(BlueprintCallable)
	void set_object_instance_data(const F_object_instance_data& data);

	/**
	 * thread safe insertion into set_list
	 */
	UFUNCTION(BlueprintCallable)
	void set_object_instance_colored_box(const F_object_instance_colored_box& data);

	/**
	 * thread safe insertion into delete_list
	 */
	UFUNCTION(BlueprintCallable)
	void delete_object(const FString& id);

	/**
	* Sets assignment mode for selection client (HUMAN, ROBOT, UNASSIGNED)
	*/
	UFUNCTION(BlueprintCallable)
	void set_assignment_mode(assignment_type assignment);

	/**
	* Gets current assignment mode for selection client
	*/
	UFUNCTION(BlueprintPure)
	assignment_type get_assignment_mode() const;

	/**
	* Selects mesh-object by its id
	*/
	UFUNCTION(BlueprintCallable)
	void select_mesh_by_actor(A_procedural_mesh_actor* actor);

	/**
	 * thread safe update of anchor
	 *
	 * @param anchor_transform new transform for the workspace anchor
	 *
	 * @attend side-effect deletes old anchor and safes new one
	 */
	UFUNCTION(BlueprintCallable)
	void update_anchor_transform(const FTransform& anchor_transform);

	/**
	 * synchronizes objects with server and subscribes to object changes
	 * if not already synced and if object_client is valid
	 */
	UFUNCTION(BlueprintCallable)
	void sync_and_subscribe(bool forced = false);
	
	/*
     * refreshes scenario mode from selection client
     * @return true if a valid scenario was applied
     */
	UFUNCTION(BlueprintCallable)
	bool refresh_scenario();

	/**
	 * gets current scenario mode from selection client
	 */
	UFUNCTION(BlueprintPure)
	scenario_type get_scenario_mode() const;

	/**
	 * checks whether given assignment is allowed in current scenario
	 */
	UFUNCTION(BlueprintPure)
	bool is_assignment_allowed(assignment_type assignment) const;

	/**
	 * checks whether rhe scenario is ready
	 */
	UFUNCTION()
	bool is_scenario_ready() const;

	/**
	 * Map of all active actors in the scene by id
	 */
	UPROPERTY(BlueprintReadOnly)
	TMap<FString, A_procedural_mesh_actor*> actors;

	/**
	 * internal clients
	 */
	UPROPERTY(BlueprintReadOnly)
	U_debug_client* debug_client;

	UPROPERTY(BlueprintReadOnly)
	U_object_client* object_client;

	UPROPERTY(BlueprintReadOnly)
	U_selection_client* selection_client;

	UPROPERTY(BlueprintReadOnly)
	A_hand_tracking_client* hand_tracking_client;

	UPROPERTY(BlueprintReadOnly)
	U_mesh_client* mesh_client;

	UPROPERTY(BlueprintReadOnly)
	A_pcl_client* pcl_client;

	UPROPERTY(BlueprintReadOnly)
	U_franka_client* franka_client;

	UPROPERTY(BlueprintReadOnly)
	U_franka_tcp_client* franka_tcp_client;

	//UPROPERTY(BlueprintReadOnly)
	//U_franka_joint_client* franka_joint_client;

	UPROPERTY(BlueprintReadOnly)
	U_franka_joint_sync_client* franka_joint_sync_client;

	UPROPERTY(BlueprintReadOnly)
	A_franka_voxel* franka_voxel;

	UPROPERTY(BlueprintReadOnly)
	A_franka_tcps* franka_tcps;

	UPROPERTY(BlueprintReadOnly)
	AFranka* franka;

	UPROPERTY(BlueprintReadOnly)
	U_franka_shadow_controller* franka_controller_;

	UPROPERTY(BlueprintAssignable)
	F_post_actors_delegate on_post_actors;

	UPROPERTY(BlueprintReadOnly)
	bool enable_registration =
#ifdef WITH_POINTCLOUD
		true;
#else
		false;
#endif

private:

	/**
	 * spawn mesh actor by its id
	 */
	A_procedural_mesh_actor* spawn_mesh_actor(const FString& id);

	/**
	 * spawn mesh actor with @ref{spawn_mesh_actor} or returns existing one
	 */
	A_procedural_mesh_actor* find_or_spawn(const FString& id);

	/**
	 * updates cached meshes for pending prototypes
	 */
	void update_meshes(const TSet<FString>& pending_proto);

	/**
	 * updates/deletes actors based on to_delete list
	 */
	void update_actors(const TArray<FString>& to_delete);

	/**
	 * handles object instance spawning/updating
	 */
	void handle_object_instance(const F_object_instance& instance);

	/**
	 * initializes anchor pin and pin component
	 */
	void init();

	/**
	 * retrieves object instance id from variant
	 */
	static FString get_object_instance_id(const F_object_instance& data);

	/*
	 * Retrieves prototype and corresponding mesh if possible
	 * @returns false if neither is present
	 * proto_id[in]; proto[out]; mesh[out]
	 */
	bool get_prototype_and_mesh(const FString& proto_id, const F_object_prototype*& proto, const F_mesh_data*& mesh);

	/**
	 * creates procedural mesh for a prototype and its mesh
	 */
	static F_procedural_mesh_data create_proc_mesh_data(const F_object_prototype& proto, const F_mesh_data& mesh);

	/**
	 * removes invalid assignment requests based on current scenario
	 */
	assignment_type sanitize_assignment(assignment_type requested) const;

	/**
	 * handle voxels!
	 * TODO:: connect delegate, test, pin to position of robot relative to the anchor!
	 */
	UFUNCTION()
	void handle_voxels(const F_voxel_data& data);

	/**
	 * handle tcps!
	 */
	UFUNCTION()
	void handle_tcps(const TArray<FVector>& data);

	/**
	 * handle joints!
	 */
	UFUNCTION()
	void handle_joints(const FFrankaJoints& data);

	/**
	 * handle synced joints!
	 */
	UFUNCTION()
	void handle_sync_joints(const TArray<F_joints_synced>& data);

	/**
	 * indicates whether synchronization has already happened
	 */
	bool synced_ = false;

	/**
	 * old target for channel change detection
	 */
	FString old_target_ = "";

	/**
	 * indicates whether application is starting
	 * used for loading existing anchor pin at startup
	 */
	bool starting_ = UARBlueprintLibrary::IsARPinLocalStoreSupported();

	/**
	 * save name of the anchor
	 */
	FName pin_save_name_ = "ROBOT_AR_PIN";

	/**
	 * set uncached prototypes
	 */
	TSet<FString> pending_prototypes_;

	/**
	 * lists for object updates
	 */
	TArray<F_object_instance> set_list_;
	TArray<FString> delete_list_;

	/**
	 * mutexes for asynchronous actor/anchor manipulation
	 */
	std::mutex delete_mutex_;
	std::mutex actor_mutex_;
	std::mutex anchor_mutex_;

	/**
	 * workspace anchor
	 */
	UPROPERTY()
	UARPin* anchor_pin_;

	/**
	 * component pinned to workspace anchor
	 * every workspace actor is connected to this
	 * component
	 */
	UPROPERTY()
	USceneComponent* pin_component_;

	/**
	 * correction component for adjusting workspace origin
	 * to better fit real world
	 */
	UPROPERTY()
	USceneComponent* correction_component_;

	/**
	 * Map of cached meshes by their name
	 */
	UPROPERTY()
	TMap<FString, F_mesh_data> meshes_;

	/**
	 * Cache of all received object instances (incl. PN-ID)
	 * -Used to resolve Petri-Net element selection on the HoloLens.
	 */
	UPROPERTY()
	TMap<FString, F_object_instance_data> object_instances_;

	/**
	 * Cache of all received box instances (incl. PN-ID corresponding to a given mesh!)
	 * -Used to resolve Petri-Net element selection on the HoloLens.
	 */
	UPROPERTY()
	TMap<FString, F_object_instance_colored_box> box_instances_;

	/**
	 * Map of cached object prototypes by their name
	 */
	UPROPERTY()
	TMap<FString, F_object_prototype> object_prototypes_;

	/**
	 * the applications channel
	 */
	UPROPERTY()
	U_grpc_channel* channel_ = nullptr;

	/**
	 * current assignment mode for selection client
	 */
	UPROPERTY()
	assignment_type current_assignment_ = assignment_type::UNASSIGNED;

	/**
	 * current scenario mode
	 */
	UPROPERTY()
	scenario_type scenario_mode_ = scenario_type::MIXED;

	/**
	 * flag to check if scenario is set successfully
	 */
	UPROPERTY()
	bool scenario_ready_ = false;

	/**
	 * removes const ref from type signature
	 *
	 * provides easier way to set value of TVariant
	 * may be redundant with at this time not available methods of new c++ standards
	 */
	template<typename T>
	struct deduce_type
	{
		using type = std::remove_const_t<std::remove_reference_t<T>>;
	};

	// only for testing purposes

	scenario_type scenario_override = scenario_type::DELEGATE_ONLY;

};