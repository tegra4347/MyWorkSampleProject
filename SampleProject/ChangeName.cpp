// 클라이언트에서 요청하고 데디 서버에서 처리
void AJCPlayerState::UserNameChange_CS_Implementation(const FString& changeName)
{
	auto playerCtrl = Cast<AJCPlayerController>(GetOwner());
	if (playerCtrl == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("AJCPlayerState::UserNameChange_CS_Implementation() - PlayerController == nullptr !"));
		UserNameChange_SC("", 0, SERVER_ERROR_CODES::UserNameChange_NotFoundPlayerCtrl);
		return;
	}

	if (changeName.IsEmpty() == true)
	{
		UserNameChange_SC("", 0, SERVER_ERROR_CODES::UserNameChange_ChangeNameEmpty);
		return;
	}

	if (playerCtrl->GetUserName().Equals(changeName))
	{
		UserNameChange_SC("", 0, SERVER_ERROR_CODES::UserNameChange_SameNickName);
		return;
	}

	// 스쿼드 및 파티에 가입되어 있는지 확인
	if (playerCtrl->GetIsSquad() == true)
	{
		UserNameChange_SC("", 0, SERVER_ERROR_CODES::UserNameChange_SquadState);
		return;
	}

	if (playerCtrl->GetIsParty() == true)
	{
		UserNameChange_SC("", 0, SERVER_ERROR_CODES::UserNameChange_PartyState);
		return;
	}
	// 현재 캐릭터가 컨텐츠 모드에 입장 했는지 체크
	if (Util::GetCombatType(GetWorld()) != ECombatType::Field)
	{
		UserNameChange_SC("", 0, SERVER_ERROR_CODES::UserNameChange_NotVillage);
		return;
	}
	// 비속어 체크
	bool isCheck = UJCCommunityComponent::CheckAccountSlangFilter(ESlangFilterType::NickName, changeName);
	if (isCheck == false)
	{
		TRACE_WARN("UserNameChange_CS NickName is fail");
		UserNameChange_SC("", 0, SERVER_ERROR_CODES::CommonIncludeForbiddenWords);
		return;
	}

	auto playerState = playerCtrl->GetPlayerState<AJCPlayerState>();

	if (!playerState)
		return;

	TArray<FWeb_Currency> Currencies;

	FWeb_Currency Currency;

	Currency.Type = static_cast<uint8>(GetDataCenter().GlobalVariableNode.NickNameEditCurrencyDataNode.NickName_Edit_Currency_Type);
	// 처음 닉네임 변경일 경우 무료
	if (playerState->NameChangeCnt == 0)
		Currency.Amount = GetDataCenter().GlobalVariableNode.NickNameEditCurrencyDataNode.NickName_Edit_Currency_Value_First;
	else // 아닌 경우 소모되는 재화 체크
		Currency.Amount = GetDataCenter().GlobalVariableNode.NickNameEditCurrencyDataNode.NickName_Edit_Currency_Value;

	if (Currency.Amount > 0)
		Currencies.Add(Currency);

	// 패킷을 보내기
	FRequestUserNameChange req;
	req.ChangeName = changeName;
	req.Currencies = Currencies;
	req.PlayerIpAddress = playerCtrl->GetPlayerNetworkAddress();
	playerCtrl->GetPacketManager()->RequestUserNameChange(&req);
}

void AJCPlayerState::UserNameChange_SC_Implementation(const FString& changeName, int UpdNameChangeCnt, int ErrorCode)
{
	UE_LOG(LogTemp, Warning, TEXT("AJCPlayerState::UserNameChange_SC_Implementation() - PlayerController == nullptr !"));

	if (ErrorCode < 0)
		Util::HandleContentError(GetWorld(), ErrorCode);
	else
	{
		// 닉네임 사용 되는 팝업 위젯에 닉네임 업데이트
		if (auto Container = UUtilUI::GetPopupWidgetContainer(GetWorld()))
		{
			if (auto Popup = Container->FindPopupWidget<UJCUserInfoPopUp>())
				Popup->UpdateUserName(changeName);
		}
	}
}