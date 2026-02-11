#include "assignment_menu_actor.h"

#include "integration_game_state.h"
#include "procedural_mesh_actor.h"
#include "Input/UxtPointerComponent.h"
#include "UObject/ConstructorHelpers.h"

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
    back_plate->SetVisibility(false);

    // set scale
    back_plate->SetRelativeScale3D(FVector(0.8f));

    // configure follow component
    follow_ = CreateDefaultSubobject<UUxtFollowComponent>(TEXT("follow"));
    follow_->OrientationType = EUxtFollowOrientBehavior::FaceCamera;
    
    // tighten distance clamp to keep it close
    follow_->bIgnoreDistanceClamp = false;
    follow_->DefaultDistance = 45.f;
    follow_->MinimumDistance = 40.f;
    follow_->MaximumDistance = 50.f;
    follow_->VerticalMaxDistance = 0.f;
    follow_->bUseFixedVerticalOffset = false;
    
    // tighten angle clamp so it stays near the center of view
    follow_->bIgnoreAngleClamp = false;
    follow_->MaxViewHorizontalDegrees = 5.f;
    follow_->MaxViewVerticalDegrees = 5.f;
    follow_->bIgnoreCameraPitchAndRoll = false;
    
    //recenter
    follow_->bInterpolatePose = true;
    follow_->LerpTime = 0.05f;

    static ConstructorHelpers::FClassFinder<AUxtPressableButtonActor> robot_button_bp(TEXT("Blueprint'/Game/robot_button_bp.robot_button_bp_C'"));
    if (robot_button_bp.Succeeded())
    {
        robot_button_class = robot_button_bp.Class;
        UE_LOG(LogTemp, Log, TEXT("Loaded robot button %s"), *robot_button_class->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[assignment_menu_actor] Failed to load robot_button_bp."));
    }

    static ConstructorHelpers::FClassFinder<AUxtPressableButtonActor> human_button_bp(TEXT("Blueprint'/Game/human_button_bp.human_button_bp_C'"));
    if (human_button_bp.Succeeded())
    {
        human_button_class = human_button_bp.Class;
        UE_LOG(LogTemp, Log, TEXT("Loaded human button %s"), *human_button_class->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[assignment_menu_actor] Failed to load human_button_bp."));
    }

    static ConstructorHelpers::FClassFinder<AUxtPressableButtonActor> unassign_button_bp(TEXT("Blueprint'/Game/unassign_button_bp.unassign_button_bp_C'"));
    if (unassign_button_bp.Succeeded())
    {
        unassign_button_class = unassign_button_bp.Class;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[assignment_menu_actor] Failed to load unassign_button_bp."));
    }
}

void A_assignment_menu_actor::initialise(A_procedural_mesh_actor* in_parent)
{
    parent_block_ = in_parent;

    if (parent_block_.IsValid())
    {
        parent_block_->OnDestroyed.AddDynamic(this, &A_assignment_menu_actor::handle_parent_destroyed);
    }

    if (APlayerController* pc = UGameplayStatics::GetPlayerController(this, 0))
    {
        cached_game_state_ = pc->GetWorld()->GetGameState<A_integration_game_state>();
        if (follow_) follow_->ActorToFollow = pc->PlayerCameraManager;
    }
    if (follow_) follow_->Recenter();

    // build buttons when game state ready and after connect/scenario refresh
    build_buttons();
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

}

void A_assignment_menu_actor::close_menu()
{
    auto destroy_button = 
    [](AUxtPressableButtonActor*& button)
    {
        if (button)
        {
            button->Destroy();
            button = nullptr;
        }
    };

    destroy_button(robot_button_instance);
    destroy_button(human_button_instance);
    destroy_button(unassign_button_instance);

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

void A_assignment_menu_actor::handle_parent_destroyed(AActor* destroyed_actor)
{
    close_menu();
}

void A_assignment_menu_actor::handle_assignment(assignment_type assignment)
{
	// closes menu if game state or parent block are invalid.
    if (!cached_game_state_.IsValid() || !parent_block_.IsValid())
    {
        close_menu();
        return;
    }

    if (!cached_game_state_->is_assignment_allowed(assignment))
    {
        UE_LOG(LogTemp, Warning, TEXT("[assignment_menu_actor] Assignment %d not allowed in current scenario."), static_cast<int32>(assignment));

        // notify parent block that menu is closed.
        parent_block_->on_assignment_menu_closed();

        // close menu after assignment.
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

void A_assignment_menu_actor::build_buttons()
{
    // clean up any existing buttons
    auto destroy_button = 
    [](AUxtPressableButtonActor*& button)
    {
        if (button)
        {
            button->Destroy();
            button = nullptr;
        }
    };
    destroy_button(robot_button_instance);
    destroy_button(human_button_instance);
    destroy_button(unassign_button_instance);

    // spawn buttons by allowed assignments
    spawn_button(unassign_button_class, unassign_button_instance, FVector(0.f, 0.f, 6.f));

    // ensure game state is available
    if (!cached_game_state_.IsValid())
    {
        if (A_integration_game_state* gs = GetWorld()->GetGameState<A_integration_game_state>())
        {
            cached_game_state_ = gs;
        }
    }

    bool allow_robot = false;
    bool allow_human = false;
    if (cached_game_state_.IsValid())
    {
        // pull latest scenario before evaluating buttons
        cached_game_state_->refresh_scenario();

        allow_robot = cached_game_state_->is_assignment_allowed(assignment_type::ROBOT);
        allow_human = cached_game_state_->is_assignment_allowed(assignment_type::HUMAN);
    }

    if (allow_robot && !allow_human)
    {
        spawn_button(robot_button_class, robot_button_instance, FVector(0.f, 0.f, 0.f));
    }
    else if (!allow_robot && allow_human)
    {
        spawn_button(human_button_class, human_button_instance, FVector(0.f, 0.f, 0.f));
    }
    else if (allow_robot && allow_human)
    {
        spawn_button(robot_button_class, robot_button_instance, FVector(0.f, 0.f, 0.f));
        spawn_button(human_button_class, human_button_instance, FVector(0.f, 0.f, -6.f));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[assignment_menu_actor] Scenario unavailable; showing only 'UNASSIGN'"));
    }

    // bind button events
    if (robot_button_instance)
    {
        if (UUxtPressableButtonComponent* Button = robot_button_instance->GetButtonComponent())
        {
            Button->OnButtonPressed.AddDynamic(this, &A_assignment_menu_actor::on_robot_pressed);
        }
    }

    if (human_button_instance)
    {
        if (UUxtPressableButtonComponent* Button = human_button_instance->GetButtonComponent())
        {
            Button->OnButtonPressed.AddDynamic(this, &A_assignment_menu_actor::on_human_pressed);
        }
    }

    if (unassign_button_instance)
    {
        if (UUxtPressableButtonComponent* Button = unassign_button_instance->GetButtonComponent())
        {
            Button->OnButtonPressed.AddDynamic(this, &A_assignment_menu_actor::on_unassign_pressed);
        }
    }
}

void A_assignment_menu_actor::spawn_button
(
    TSubclassOf<AUxtPressableButtonActor> button_class,
    AUxtPressableButtonActor*& out_instance,
    const FVector& relative_location
)
{
    if (!button_class)
    {
        button_class = AUxtPressableButtonActor::StaticClass();
    }

    if (!back_plate) return;

    FActorSpawnParameters params;
    params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    params.Owner = this;

    out_instance = GetWorld()->SpawnActor<AUxtPressableButtonActor>(button_class, params);
    if (!out_instance) return;
    out_instance->SetActorScale3D(FVector(1.5f));
    out_instance->AttachToComponent(back_plate, FAttachmentTransformRules::KeepRelativeTransform);
    out_instance->SetActorRelativeLocation(relative_location);
}

