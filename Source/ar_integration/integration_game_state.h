#pragma once

#include "ARPin.h"
#include "ARBlueprintLibrary.h"
#include "GameFramework/GameStateBase.h"

#include "grpc_channel.h"
#include "debug_client.h"
#include "object_client.h"
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

	void update_meshes(const TSet<FString>& pending_proto);
	void update_actors(const TArray<FString>& to_delete);
	
	void handle_object_instance(const F_object_instance& instance);

	void init();

	static FString get_object_instance_id(const F_object_instance& data);

	/*
	 * Retrieves prototype and corresponding mesh if possible
	 * @returns false if neither is present
	 * proto_id[in]; proto[out]; mesh[out]
	 */
	bool get_prototype_and_mesh(
		const FString& proto_id, 
		const F_object_prototype*& proto, 
		const F_mesh_data*& mesh);

	/**
	 * creates procedural mesh for a prototype and its mesh
	 */
	static F_procedural_mesh_data create_proc_mesh_data(
		const F_object_prototype& proto,
		const F_mesh_data& mesh);

	bool synced = false;
	FString old_target = "";

	/**
	 * workspace anchor
	 */
	UPROPERTY()
	UARPin* anchor_pin;

	/**
	 * component pinned to workspace anchor
	 * every workspace actor is connected to this
	 * component
	 */
	UPROPERTY()
	USceneComponent* pin_component;

	UPROPERTY()
	USceneComponent* correction_component;

	/**
	 * Map of cached meshes by their name
	 */
	UPROPERTY()
	TMap<FString, F_mesh_data> meshes;

	/**
	 * Map of cached object prototypes by their name
	 */
	UPROPERTY()
	TMap<FString, F_object_prototype> object_prototypes;

	/**
	 * the applications channel
	 */
	UPROPERTY()
	U_grpc_channel* channel = nullptr;

	bool starting = UARBlueprintLibrary::IsARPinLocalStoreSupported();

	/**
	 * save name of the anchor
	 */
	FName pin_save_name = "ROBOT_AR_PIN";

	/**
	 * mutexes for asynchronous actor/anchor manipulation 
	 */
	std::mutex delete_mutex;
	std::mutex actor_mutex;
	std::mutex anchor_mutex;

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

	/**
	 * spawn mesh actor by its id
	 */
	A_procedural_mesh_actor* spawn_mesh_actor(const FString& id);

	/**
	 * spawn mesh actor with @ref{spawn_mesh_actor} or returns existing one
	 */
	A_procedural_mesh_actor* find_or_spawn(const FString& id);

	/**
	 * handle voxels!
	 * TODO:: connect delegate, test, pin to position of robot relative to the anchor!
	 */
	UFUNCTION()
	void handle_voxels(const F_voxel_data& data);

	UFUNCTION()
	void handle_tcps(const TArray<FVector>& data);

	UFUNCTION()
	void handle_joints(const FFrankaJoints& data);

	UFUNCTION()
	void handle_sync_joints(const TArray<F_joints_synced>& data);


	/**
	 * set uncached prototypes
	 */
	TSet<FString> pending_prototypes;

	/**
	 * lists for object updates
	 */
	TArray<F_object_instance> set_list;
	TArray<FString> delete_list;
};