#pragma once

#include "Math/Box.h"

#include "Franka.h"

#include "grpc_wrapper.generated.h"

/**
 * @struct F_mesh_data
 *
 * struct wrapping data needed for
 * procedural mesh generation
 */
USTRUCT(BlueprintType)
struct AR_INTEGRATION_API F_mesh_data
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	TArray<FVector> vertices;

	UPROPERTY(VisibleAnywhere)
	TArray<int32> indices;

	UPROPERTY(VisibleAnywhere)
	FString name;

	UPROPERTY(VisibleAnywhere)
	TArray<FVector> normals;

	UPROPERTY(VisibleAnywhere)
	TArray<FColor> colors;
};

/**
 * @struct F_obb
 *
 * wrapper for a oriented bounding box
 */
USTRUCT(BlueprintType)
struct AR_INTEGRATION_API F_obb
{
	GENERATED_BODY()

	UPROPERTY()
	FBox axis_box = {};

	UPROPERTY()
	FQuat rotation = FQuat::Identity;
};

/**
 * @struct F_colored_box
 *
 * wrapper for a wireframe oriented bounding box
 */
USTRUCT(BlueprintType)
struct AR_INTEGRATION_API F_colored_box
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	F_obb box;

	UPROPERTY(VisibleAnywhere)
	FColor color = FColor::Red;
};

/**
 * @struct F_object_prototype
 *
 * wrapper for a object prototype
 */
USTRUCT(BlueprintType)
struct AR_INTEGRATION_API F_object_prototype
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FBox bounding_box = {};

	UPROPERTY(VisibleAnywhere)
	FColor mean_color = FColor::Red;

	UPROPERTY(VisibleAnywhere)
	FString mesh_name;

	UPROPERTY(VisibleAnywhere)
	FString name;

	UPROPERTY(VisibleAnywhere)
	FString type;
};

/**
 * @struct F_object_data
 *
 * wrapper for a object with a pose
 */
USTRUCT(BlueprintType)
struct AR_INTEGRATION_API F_object_data
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FString prototype_name;

	UPROPERTY(VisibleAnywhere)
	FTransform transform = FTransform::Identity;
};

/*
 * Two different struct must be created
 * since TVariant is not supported by
 * UPROPERTY -> not supported by delegates
 */
USTRUCT(BlueprintType)
struct AR_INTEGRATION_API F_object_instance_data
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FString id;

	UPROPERTY(VisibleAnywhere)
	int32 pn_id;

	UPROPERTY(VisibleAnywhere)
	F_object_data data;
};

USTRUCT(BlueprintType)
struct AR_INTEGRATION_API F_object_instance_colored_box
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FString id;

	UPROPERTY(VisibleAnywhere)
	int32 pn_id;

	UPROPERTY(VisibleAnywhere)
	F_colored_box data;
};

/**
 * @class U_grpc_wrapper
 *
 * wrapper for methods and objects which can't be realized with
 * structs
 */
UCLASS()
class AR_INTEGRATION_API U_grpc_wrapper : public UObject
{
	GENERATED_BODY()
public:

	/**
	 * creates a oriented bounding box from its parameters
	 *
	 * @param translation position of the obb
	 * @param rotation rotation of the obb
	 * @param extent half size of the bounding box
	 */
	UFUNCTION(BlueprintCallable)
	static F_obb Make_obb(FVector translation, FRotator rotation, FVector extent);
};

USTRUCT(BlueprintType)
struct AR_INTEGRATION_API F_voxel_data
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GRPC Wrapper")
	TArray<FVector> indices = {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GRPC Wrapper")
	float voxel_side_length = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GRPC Wrapper")
	FTransform robot_origin = FTransform::Identity;
};

USTRUCT(Blueprintable)
struct AR_INTEGRATION_API F_joints_synced
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Robot")
	FFrankaJoints joints {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Robot")
	FDateTime time_stamp {};
};

UENUM(BlueprintType)
enum Visual_Change : uint8
{
	ENABLED = 0     UMETA(DisplayName = "Enabled"),
	DISABLED = 1	UMETA(DisplayName = "Disabled"),
	REVOKED = 2     UMETA(DisplayName = "Revoked")
};

typedef TVariant<TArray<F_joints_synced>, Visual_Change> Sync_Joints_Data;
typedef TVariant<TArray<FVector>, Visual_Change> Tcps_Data;
typedef TVariant<F_voxel_data, Visual_Change> Voxel_Data;