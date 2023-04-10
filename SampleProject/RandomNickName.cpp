void UCreateNickNameWidget::OnClickNickNameShuffle()
{
	auto NickNameDataTableArray = FNickNameDataTable::GetAll();

	if (NickNameDataTableArray.Num() == 0)
		return;

	int NickNameCount = NickNameDataTableArray.Num();

	int RandomIndex = rand() % NickNameCount;

	// 동일한 닉네임이 나올 경우 셔플 추가 진행
	if (NickNameCount > 1)
	{
		while (true)
		{
			if (RandomIndex != CurrentShuffleIndex)
				break;
			else
				RandomIndex = rand() % NickNameCount;
		}
	}

	auto FindElem = NickNameDataTableArray[RandomIndex];

	// 자동닉네임 테이블에 해당 정보가 있을 경우
	if (FindElem)
	{
		auto TempText = FUITextTable::GetString(FindElem->Nickname_String_ID);
		if (TempText.Len() < GetDataCenter().GlobalVariableNode.CharacterNode.InformationNode.NameLengthNode.Min ||
			TempText.Len() > GetDataCenter().GlobalVariableNode.CharacterNode.InformationNode.NameLengthNode.Max)
		{
			return;
		}

		if (NickNameTextBlock)
		{
			NickName = TempText;
			NickNameTextBlock->SetText(FText::FromString(TempText));
		}

		CurrentShuffleIndex = RandomIndex;
	}

	// 버튼 딜레이 초기화
	StartRefreshTime();
}

// 버튼 딜레이 시간 초기화
void UCreateNickNameWidget::StartRefreshTime()
{
	RefreshButtonTime = GetDataCenter().GlobalVariableNode.NickNameEditCurrencyDataNode.Nickname_Random_Dice_Refresh_Time;

	if (RefreshTimeText)
	{
		RefreshTimeText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		RefreshTimeText->SetText(FText::AsNumber(RefreshButtonTime));
	}

	GetWorld()->GetTimerManager().SetTimer(
		TimerHandler_RefreshTime
		, this
		, &UCreateNickNameWidget::UpdateRefreshTime
		, 1.0f, true);
}
// 버튼 딜레이 타임 업데이트
void UCreateNickNameWidget::UpdateRefreshTime()
{
	--RefreshButtonTime;

	if (RefreshButtonTime <= 0)
	{
		if (RefreshTimeText)
			RefreshTimeText->SetVisibility(ESlateVisibility::Collapsed);

		if (TimerHandler_RefreshTime.IsValid())
			GetWorld()->GetTimerManager().ClearTimer(TimerHandler_RefreshTime);
	}
	else
		RefreshTimeText->SetText(FText::AsNumber(RefreshButtonTime));
}