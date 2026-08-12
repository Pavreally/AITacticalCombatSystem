// Pavel Gornostaev <https://github.com/Pavreally>

#include "Details/AITCSTacticalObjectCustomization.h"
#include "PropertyHandle.h"
#include "DetailWidgetRow.h"
#include "DetailLayoutBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "Widgets/Text/STextBlock.h"
#include "Modules/ModuleManager.h"

#include "Data/AITCSCoreData.h"

TSharedRef<IPropertyTypeCustomization> FAITCSTacticalObjectCustomization::MakeInstance()
{
    return MakeShared<FAITCSTacticalObjectCustomization>();
}

void FAITCSTacticalObjectCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    HeaderRow
        .NameContent()
        [
            StructPropertyHandle->CreatePropertyNameWidget()
        ]
        .ValueContent()
        .MaxDesiredWidth(600.0f)
        [
            SNew(STextBlock).Text(FText::GetEmpty())
        ];
}

void FAITCSTacticalObjectCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    bool bIsSimpleObject = false;
    const TSharedPtr<IPropertyHandle> IsSimpleObjectHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAITCSTacticalObject, bIsSimpleObject));
    if (IsSimpleObjectHandle.IsValid())
    {
        IsSimpleObjectHandle->GetValue(bIsSimpleObject);
    }

    // Present a curated order of properties and ensure ExternalExtensions is visible
    auto AddChildByName = [&](const FName& Name)
    {
        TSharedPtr<IPropertyHandle> Child = StructPropertyHandle->GetChildHandle(Name);
        if (Child.IsValid())
        {
            ChildBuilder.AddProperty(Child.ToSharedRef());
        }
    };

    AddChildByName(GET_MEMBER_NAME_CHECKED(FAITCSTacticalObject, DisplayName));
    AddChildByName(GET_MEMBER_NAME_CHECKED(FAITCSTacticalObject, Attitude));
    AddChildByName(GET_MEMBER_NAME_CHECKED(FAITCSTacticalObject, Shape));
    AddChildByName(GET_MEMBER_NAME_CHECKED(FAITCSTacticalObject, Color));
    AddChildByName(GET_MEMBER_NAME_CHECKED(FAITCSTacticalObject, PreferredDirectionDegrees));
    AddChildByName(GET_MEMBER_NAME_CHECKED(FAITCSTacticalObject, DirectionSpace));
    AddChildByName(GET_MEMBER_NAME_CHECKED(FAITCSTacticalObject, DesiredPlayerDistance));
    AddChildByName(GET_MEMBER_NAME_CHECKED(FAITCSTacticalObject, AttackTargetDistance));
    AddChildByName(GET_MEMBER_NAME_CHECKED(FAITCSTacticalObject, bUseAttackTargetDistanceAsFormationOverride));
    AddChildByName(GET_MEMBER_NAME_CHECKED(FAITCSTacticalObject, PlayerEngagementRadius));
    AddChildByName(GET_MEMBER_NAME_CHECKED(FAITCSTacticalObject, bReturnToInitialLocationOutsideEngagementRadius));
    if (!bIsSimpleObject)
    {
        AddChildByName(GET_MEMBER_NAME_CHECKED(FAITCSTacticalObject, GroupAssets));
        AddChildByName(GET_MEMBER_NAME_CHECKED(FAITCSTacticalObject, ActiveGroupIndex));
    }
    AddChildByName(GET_MEMBER_NAME_CHECKED(FAITCSTacticalObject, RoleTag));
    AddChildByName(GET_MEMBER_NAME_CHECKED(FAITCSTacticalObject, PawnClass));
    AddChildByName(GET_MEMBER_NAME_CHECKED(FAITCSTacticalObject, AIControllerClass));

    // Integration / External extensions
    AddChildByName(GET_MEMBER_NAME_CHECKED(FAITCSTacticalObject, ExternalExtensions));

    AddChildByName(GET_MEMBER_NAME_CHECKED(FAITCSTacticalObject, Comment));
}
