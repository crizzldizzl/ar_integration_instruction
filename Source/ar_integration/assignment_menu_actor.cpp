#include "assignment_menu_actor.h"

#include "integration_game_state.h"
#include "procedural_mesh_actor.h"
#include "Input/UxtPointerComponent.h"

A_assignment_menu_actor::A_assignment_menu_actor()
{
    PrimaryActorTick.bCanEverTick = true;

    root_ = CreateDefaultSubobject<USceneComponent>(TEXT("root"));
    SetRootComponent(root_);

    // give the menu a small translucent backing plate for visibility.
    back_plate = CreateDefaultSubobject<UUxtBackPlateComponent>(TEXT("back_plate"));
    back_plate->SetupAttachment(root_);
    back_plate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    back_plate->SetMobility(EComponentMobility::Movable);
    back_plate->bEditableWhenInherited = true;

    // create child actor components that instantiate pressable buttons.
    robot_button_component = CreateDefaultSubobject<UChildActorComponent>(TEXT("robot_button"));
    robot_button_component->SetupAttachment(back_plate);
    robot_button_component->SetRelativeLocation(FVector(0.f, 0.f, 6.f));
    robot_button_component->SetChildActorClass(AUxtPressableButtonActor::StaticClass());

    human_button_component = CreateDefaultSubobject<UChildActorComponent>(TEXT("human_button"));
    human_button_component->SetupAttachment(back_plate);
    human_button_component->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
    human_button_component->SetChildActorClass(AUxtPressableButtonActor::StaticClass());

    unassign_button_component = CreateDefaultSubobject<UChildActorComponent>(TEXT("unassign_button"));
    unassign_button_component->SetupAttachment(back_plate);
    unassign_button_component->SetRelativeLocation(FVector(0.f, 0.f, -6.f));
    unassign_button_component->SetChildActorClass(AUxtPressableButtonActor::StaticClass());
}

void A_assignment_menu_actor::initialise(A_procedural_mesh_actor* in_parent)
{
    parent_block_ = in_parent;

    if (APlayerController* player_controller = UGameplayStatics::GetPlayerController(this, 0))
    {
        cached_game_state_ = player_controller->GetWorld()->GetGameState<A_integration_game_state>();
    }

    // configure buttons.
    if (auto* robot_button_actor = Cast<AUxtPressableButtonActor>(robot_button_component->GetChildActor()))
    {
        robot_button_actor->GetButtonComponent()->OnButtonPressed.AddDynamic(this, &A_assignment_menu_actor::on_robot_pressed);
    }

    if (auto* human_button_actor = Cast<AUxtPressableButtonActor>(human_button_component->GetChildActor()))
    {
        human_button_actor->GetButtonComponent()->OnButtonPressed.AddDynamic(this, &A_assignment_menu_actor::on_human_pressed);
    }

    if (auto* unassign_button_actor = Cast<AUxtPressableButtonActor>(unassign_button_component->GetChildActor()))
    {
        unassign_button_actor->GetButtonComponent()->OnButtonPressed.AddDynamic(this, &A_assignment_menu_actor::on_unassign_pressed);
    }

    if (APlayerCameraManager* camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
    {
        FVector camera_location = camera->GetCameraLocation();
        FRotator camera_rotation = camera->GetCameraRotation();
        FTransform camera_world(camera_rotation, camera_location);

        camera_offset_local_ = camera_world.InverseTransformPosition(GetActorLocation());

		camera_offset_local_ *= camera_offset_multiplier_;

        has_camera_relative_offset_ = true;
    }
}

void A_assignment_menu_actor::BeginPlay()
{
    Super::BeginPlay();
}

void A_assignment_menu_actor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // if parent disappears, close menu.
    if (!parent_block_.IsValid())
    {
        Destroy();
        return;
    }

	// maintain camera-relative offset if applicable.
    if (has_camera_relative_offset_)
    {
        if (APlayerCameraManager* camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
        {
            const FTransform camera_world(camera->GetCameraRotation(), camera->GetCameraLocation());
            const FVector new_location = camera_world.TransformPosition(camera_offset_local_);
            SetActorLocation(new_location);

            const FVector to_camera = camera_world.GetLocation() - new_location;
            if (!to_camera.IsNearlyZero())
            {
                SetActorRotation(to_camera.Rotation());
            }
        }
    }

}

void A_assignment_menu_actor::close_menu()
{
    Destroy();
}

void A_assignment_menu_actor::on_robot_pressed(UUxtPressableButtonComponent* button, UUxtPointerComponent* pointer)
{
    handle_assignment(assignment_type::ROBOT);
}

void A_assignment_menu_actor::on_human_pressed(UUxtPressableButtonComponent* button, UUxtPointerComponent* pointer)
{
    handle_assignment(assignment_type::HUMAN);
}

void A_assignment_menu_actor::on_unassign_pressed(UUxtPressableButtonComponent* button, UUxtPointerComponent* pointer)
{
    handle_assignment(assignment_type::UNASSIGNED);
}

void A_assignment_menu_actor::handle_assignment(assignment_type assignment)
{
	// closes menu if game state or parent block are invalid.
    if (!cached_game_state_.IsValid() || !parent_block_.IsValid())
    {
        close_menu();
        return;
    }

	// update game state and send selection to server.
    cached_game_state_->set_assignment_mode(assignment);
    cached_game_state_->select_mesh_by_actor(parent_block_.Get());

	// update parent block assignment state.
	parent_block_->set_assignment_state(assignment);

	// notify parent block that menu is closed.
    parent_block_->on_assignment_menu_closed();

	// close menu after assignment.
    close_menu();
}

