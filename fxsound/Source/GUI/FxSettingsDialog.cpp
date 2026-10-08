/*
FxSound
Copyright (C) 2025  FxSound LLC

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <JuceHeader.h>
#include "FxSettingsDialog.h"
#include "../Utils/SysInfo/SysInfo.h"

//==============================================================================
FxSettingsDialog::FxSettingsDialog() : FxWindow("Settings"), tooltip_window_(this)
{
	setContent(&settings_content_);
	centreWithSize(getWidth(), getHeight());
	addToDesktop(0);
	toFront(true);
}

void FxSettingsDialog::closeButtonPressed()
{
	exitModalState(0);
	removeFromDesktop();
}

void FxSettingsDialog::paint(Graphics& g)
{
	FxWindow::paint(g);

	g.setColour(Colour(FXCOLOR(Outline)).withAlpha(1.0f));
	g.drawLine((float)SEPARATOR_X, (float)title_bar_.getBottom(), (float)SEPARATOR_X, (float)getLocalBounds().getBottom());
}

void FxSettingsDialog::SettingsButton::paint(Graphics& g)
{
	auto bounds = getLocalBounds();

	if (getToggleState())
	{
		g.setColour(Colour(FXCOLOR(MenuHighlightBackground)).withAlpha(1.0f));
	}
	else
	{
		g.setColour(Colour(FXCOLOR(MenuBackground)).withAlpha(1.0f));
	}
	auto rect = juce::Rectangle<float>(0, 0, (float)bounds.getHeight(), (float)bounds.getHeight());
	g.fillRoundedRectangle(rect, (float)bounds.getHeight()/4);

	image_->drawWithin(g, rect.reduced(10, 10), RectanglePlacement::centred, 1.0f);

	if (getToggleState())
	{
		g.setColour(Colour(FXCOLOR(HighlightedText)).withAlpha(1.0f));
	}
	else
	{
		g.setColour(Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
	}
	auto w = bounds.getWidth() - bounds.getHeight() + 5;

	auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());
	g.setFont(theme.getNormalFont());
	g.drawText(TRANS(getName()), juce::Rectangle<int>(bounds.getHeight()+5, 0, w, bounds.getHeight()), Justification::centredLeft);
}

bool FxSettingsDialog::keyPressed(const KeyPress& key)
{
	if (key == KeyPress::escapeKey)
	{
		exitModalState(0);
		removeFromDesktop();
		return true;
	}

	return Component::keyPressed(key);
}

FxSettingsDialog::SettingsComponent::SettingsComponent()
{
	audio_button_ = std::make_unique<SettingsButton>("Audio");
	audio_button_->setToggleState(true, NotificationType::dontSendNotification);
	audio_button_->setImage(Drawable::createFromImageData(BinaryData::speaker_svg, BinaryData::speaker_svgSize).get());
	audio_button_->addListener(this);


	general_button_ = std::make_unique<SettingsButton>("General");
	general_button_->setToggleState(false, NotificationType::dontSendNotification);
	general_button_->setImage(Drawable::createFromImageData(BinaryData::settings_svg, BinaryData::settings_svgSize).get());
	general_button_->addListener(this);
	
	help_button_ = std::make_unique<SettingsButton>("Help");
	help_button_->setToggleState(false, NotificationType::dontSendNotification);
	help_button_->setImage(Drawable::createFromImageData(BinaryData::question_svg, BinaryData::question_svgSize).get());
	help_button_->addListener(this);    

	channels_button_ = std::make_unique<SettingsButton>("Channels");
	channels_button_->setToggleState(false, NotificationType::dontSendNotification);
	channels_button_->setImage(Drawable::createFromImageData(BinaryData::speaker_svg, BinaryData::speaker_svgSize).get());
	channels_button_->addListener(this);

	addAndMakeVisible(audio_button_.get());
	addAndMakeVisible(channels_button_.get());
	addAndMakeVisible(general_button_.get());
	addAndMakeVisible(help_button_.get());

	addAndMakeVisible(audio_settings_pane_);
	addChildComponent(channels_settings_pane_);
	addChildComponent(general_settings_pane_);
	addChildComponent(help_settings_pane_);

    setSize(WIDTH, HEIGHT);
}

void FxSettingsDialog::SettingsComponent::resized()
{
	audio_button_->setBounds(BUTTON_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
	channels_button_->setBounds(BUTTON_X, audio_button_->getBottom() + 20, BUTTON_WIDTH, BUTTON_HEIGHT);
	general_button_->setBounds(BUTTON_X, channels_button_->getBottom() + 20, BUTTON_WIDTH, BUTTON_HEIGHT);
	help_button_->setBounds(BUTTON_X, general_button_->getBottom() + 20, BUTTON_WIDTH, BUTTON_HEIGHT);

	juce::Rectangle<int> pane_rect(SEPARATOR_X + 1, 1, getWidth() - SEPARATOR_X + 1, getHeight() - 1);
	audio_settings_pane_.setBounds(pane_rect);
	channels_settings_pane_.setBounds(pane_rect);
	general_settings_pane_.setBounds(pane_rect);
	help_settings_pane_.setBounds(pane_rect);
}

void  FxSettingsDialog::SettingsComponent::buttonClicked(Button* button)
{
	std::pair<SettingsButton*, Component*> pages[] = { { audio_button_.get(), &audio_settings_pane_ },
		{ channels_button_.get(), &channels_settings_pane_ },
		{ general_button_.get(), &general_settings_pane_ },
		{ help_button_.get(), &help_settings_pane_ } };

	for (auto& page : pages)
	{
		page.first->setToggleState(page.first == button, NotificationType::dontSendNotification);
		page.second->setVisible(page.first == button);
	}
}

FxSettingsDialog::SettingsPane::SettingsPane(String name)
{
	name_ = name;
	auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());

    title_.setFont(theme.getTitleFont());
	title_.setText(TRANS(name_), NotificationType::dontSendNotification);	
	title_.setColour(Label::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	title_.setJustificationType(Justification::centredLeft);
	addAndMakeVisible(title_);
}

void FxSettingsDialog::SettingsPane::paint(Graphics&)
{
    auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());

    title_.setFont(theme.getTitleFont());
    title_.setText(TRANS(name_), NotificationType::dontSendNotification);    
}

FxSettingsDialog::AudioSettingsPane::AudioSettingsPane() :
	SettingsPane("Audio"),
	prioritize_new_output_toggle_(TRANS("Prioritize new output devices")),
	reset_presets_button_(TRANS("Reset presets to factory defaults"))
{
	setFocusContainer(true);

	output_preference_title_.setColour(Label::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	output_preference_title_.setJustificationType(Justification::centredLeft);

	output_preference_.setMouseCursor(MouseCursor::PointingHandCursor);
	output_preference_.setWantsKeyboardFocus(true);
	output_preference_.setEnabled(true);

	prioritize_new_output_toggle_.setMouseCursor(MouseCursor::PointingHandCursor);
	prioritize_new_output_toggle_.setColour(ToggleButton::ColourIds::tickColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	prioritize_new_output_toggle_.setColour(ToggleButton::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	prioritize_new_output_toggle_.setWantsKeyboardFocus(true);

	reset_presets_button_.setSize(RESET_PRESETS_BUTTON_WIDTH, BUTTON_HEIGHT);
	reset_presets_button_.setMouseCursor(MouseCursor::PointingHandCursor);

	prioritize_new_output_toggle_.setToggleState(FxController::getInstance().isNewOutputPrioritized(), NotificationType::dontSendNotification);
	prioritize_new_output_toggle_.onClick = [this]() { FxController::getInstance().setNewOutputPrioritized(prioritize_new_output_toggle_.getToggleState()); };

	auto preset_modified = false;
	auto preset_count = FxModel::getModel().getPresetCount();
	for (auto i = 0; i < preset_count; i++)
	{
		if (FxModel::getModel().isPresetModified(i))
		{
			preset_modified = true;
			break;
		}
	}
	reset_presets_button_.setEnabled(FxModel::getModel().getUserPresetCount() > 0 || preset_modified);

	reset_presets_button_.onClick = [this]() {
		auto& controller = FxController::getInstance();

		controller.resetPresets();
		reset_presets_button_.setEnabled(false);
		};

	setText();

	addAndMakeVisible(&output_preference_title_);
	addAndMakeVisible(&output_preference_);
	addAndMakeVisible(&prioritize_new_output_toggle_);
	addAndMakeVisible(&reset_presets_button_);
}

FxSettingsDialog::AudioSettingsPane::~AudioSettingsPane()
{
}

void FxSettingsDialog::AudioSettingsPane::resized()
{
	auto bounds = getLocalBounds().withLeft(X_MARGIN).withTop(Y_MARGIN).withHeight(TITLE_HEIGHT);
	title_.setBounds(bounds);

	output_preference_title_.setBounds(X_MARGIN, ENDPOINT_Y, LABEL_WIDTH, LABEL_HEIGHT);
	int y = output_preference_title_.getBottom() + 10;
	auto width = getWidth() - ((X_MARGIN + 5) * 2);
	output_preference_.setBounds(X_MARGIN, y, width, OUTPUT_PREFERENCE_HEIGHT);

    y = output_preference_.getBottom() + 10;
    prioritize_new_output_toggle_.setBounds(X_MARGIN, y, width, TOGGLE_BUTTON_HEIGHT);

	auto group_x = output_preference_title_.getX() - GROUP_MARGIN;
	auto group_y = output_preference_title_.getY() - GROUP_MARGIN;
	auto group_width = output_preference_.getRight() - group_x + GROUP_MARGIN;
	auto group_height = prioritize_new_output_toggle_.getBottom() - group_y + GROUP_MARGIN;
	output_preference_bounds_ = juce::Rectangle<float>(group_x, group_y, group_width, group_height);

	y = prioritize_new_output_toggle_.getBottom() + 30;
	resizeResetButton(X_MARGIN, y);
}

void FxSettingsDialog::AudioSettingsPane::paint(Graphics& g)
{
	g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

	g.setFillType(FillType(Colour(FXCOLOR(DefaultFill)).withAlpha(0.2f)));
	g.fillRoundedRectangle(output_preference_bounds_, 8);

	setText();

	SettingsPane::paint(g);
}

void FxSettingsDialog::AudioSettingsPane::setText()
{
	auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());

	output_preference_title_.setFont(theme.getNormalFont());
	output_preference_title_.setText(TRANS("Output Device Preference"), NotificationType::dontSendNotification);

	prioritize_new_output_toggle_.setButtonText(TRANS("Prioritize new output devices"));

	reset_presets_button_.setButtonText(TRANS("Reset presets to factory defaults"));
	resizeResetButton(reset_presets_button_.getX(), reset_presets_button_.getY());
}

void FxSettingsDialog::AudioSettingsPane::resizeResetButton(int x, int y)
{
	String buttonText = reset_presets_button_.getButtonText();

	int index = 0;
	int lineCount = 1;
	do {
		index = buttonText.indexOfChar(index, L'\n');
		if (index >= 0)
		{
			index++;
			lineCount++;
		}
		else
		{
			break;
		}
	} while (lineCount <= 3); // Resize the button height for upto 3 lines of text

	int buttonWidth = min(reset_presets_button_.getBestWidthForHeight(BUTTON_HEIGHT * lineCount), MAX_BUTTON_WIDTH);
	if (buttonWidth < RESET_PRESETS_BUTTON_WIDTH)
	{
		buttonWidth = RESET_PRESETS_BUTTON_WIDTH;
	}

	reset_presets_button_.setBounds(x, y, buttonWidth, BUTTON_HEIGHT * lineCount);
}

void FxSettingsDialog::AudioSettingsPane::visibilityChanged()
{
	if (isVisible())
	{
		output_preference_.update();
    }
}

void FxSettingsDialog::AudioSettingsPane::mouseEnter(const MouseEvent& mouse_event)
{
	Component::mouseEnter(mouse_event);
}

void FxSettingsDialog::AudioSettingsPane::mouseExit(const MouseEvent& mouse_event)
{
	Component::mouseEnter(mouse_event);
}

FxSettingsDialog::GeneralSettingsPane::GeneralSettingsPane() :
	SettingsPane("General Preferences"),
	launch_toggle_(TRANS("Launch on system startup")),
	hide_help_tips_toggle_(TRANS("Hide help tips for audio controls")),
	hide_notifications_toggle_(TRANS("Hide notifications")),
	hotkeys_toggle_(TRANS("Disable keyboard shortcuts"))
{
	StringArray hotKeySettingsKeys = { FxController::HK_CMD_ON_OFF, FxController::HK_CMD_OPEN_CLOSE, FxController::HK_CMD_NEXT_PRESET, FxController::HK_CMD_PREVIOUS_PRESET, FxController::HK_CMD_NEXT_OUTPUT };
	StringArray hotkey_names = { "Turn FxSound On/Off", "Open/Close FxSound",
								   "Use Next Preset", "Use Previous Preset", "Change Playback Device"};

	setFocusContainer(true);

	launch_toggle_.setMouseCursor(MouseCursor::PointingHandCursor);
	launch_toggle_.setColour(ToggleButton::ColourIds::tickColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	launch_toggle_.setColour(ToggleButton::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	launch_toggle_.setWantsKeyboardFocus(true);
	
	hide_help_tips_toggle_.setMouseCursor(MouseCursor::PointingHandCursor);
    hide_help_tips_toggle_.setColour(ToggleButton::ColourIds::tickColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
    hide_help_tips_toggle_.setColour(ToggleButton::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	hide_help_tips_toggle_.setWantsKeyboardFocus(true);
	hide_notifications_toggle_.setMouseCursor(MouseCursor::PointingHandCursor);
	hide_notifications_toggle_.setColour(ToggleButton::ColourIds::tickColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	hide_notifications_toggle_.setColour(ToggleButton::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	hide_notifications_toggle_.setWantsKeyboardFocus(true);
	hotkeys_toggle_.setMouseCursor(MouseCursor::PointingHandCursor);
	hotkeys_toggle_.setColour(ToggleButton::ColourIds::tickColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	hotkeys_toggle_.setColour(ToggleButton::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	hotkeys_toggle_.setWantsKeyboardFocus(true);

	bool hotkey_enabled = FxModel::getModel().getHotkeySupport();
	for (int i=0; i<hotkey_names.size(); i++)
	{
		auto label = new FxHotkeyLabel(hotkey_names[i], hotKeySettingsKeys[i]);
		label->setEnabled(hotkey_enabled);
		hotkey_labels_.add(label);
		addAndMakeVisible(label);
	}

    if (SysInfo::canSupportHotkeys())
    {
        hotkeys_toggle_.setToggleState(!FxModel::getModel().getHotkeySupport(), NotificationType::dontSendNotification);
    }
    else
    {
        hotkeys_toggle_.setToggleState(true, NotificationType::dontSendNotification);
        hotkeys_toggle_.setEnabled(false);
    }
	hotkeys_toggle_.onClick = [this]()  { 
											FxController::getInstance().enableHotkeys(!hotkeys_toggle_.getToggleState());
											bool enabled = FxModel::getModel().getHotkeySupport();
											for (auto& hotkey_label : hotkey_labels_)
											{
												hotkey_label->setEnabled(enabled);
											}
										};

	launch_toggle_.setToggleState(FxController::getInstance().isLaunchOnStartup(), NotificationType::dontSendNotification);
	launch_toggle_.onClick = [this]() { FxController::getInstance().setLaunchOnStartup(launch_toggle_.getToggleState()); };

    hide_help_tips_toggle_.setToggleState(FxController::getInstance().isHelpTooltipsHidden(), NotificationType::dontSendNotification);
    hide_help_tips_toggle_.onClick = [this]() { FxController::getInstance().setHelpTooltipsHidden(hide_help_tips_toggle_.getToggleState()); };
	
	hide_notifications_toggle_.setToggleState(FxController::getInstance().isNotificationsHidden(), NotificationType::dontSendNotification);
	hide_notifications_toggle_.onClick = [this]() { FxController::getInstance().setNotificationsHidden(hide_notifications_toggle_.getToggleState()); };

	auto os = SystemStats::getOperatingSystemType();
	if (os == SystemStats::OperatingSystemType::Windows7)
	{
		addAndMakeVisible(&launch_toggle_);
	}
	
	addAndMakeVisible(&hide_help_tips_toggle_);
	addAndMakeVisible(&hide_notifications_toggle_);
	addAndMakeVisible(&hotkeys_toggle_);
	addAndMakeVisible(&language_switch_);

	setText();
}

FxSettingsDialog::GeneralSettingsPane::~GeneralSettingsPane()
{
}

void FxSettingsDialog::GeneralSettingsPane::resized()
{
	auto bounds = getLocalBounds().withLeft(X_MARGIN).withTop(Y_MARGIN).withHeight(TITLE_HEIGHT);
	title_.setBounds(bounds);

    language_switch_.setBounds(X_MARGIN, LANGUAGE_SWITCH_Y, FxLanguage::WIDTH, FxLanguage::HEIGHT);

    int y = language_switch_.getBottom() + 20;
	if (launch_toggle_.isVisible())
	{
		launch_toggle_.setBounds(X_MARGIN, y, getWidth() - X_MARGIN, TOGGLE_BUTTON_HEIGHT);
		y = launch_toggle_.getBottom() + 20;
	}

    hide_help_tips_toggle_.setBounds(X_MARGIN, y, getWidth() - X_MARGIN, TOGGLE_BUTTON_HEIGHT);

	y = hide_help_tips_toggle_.getBottom() + 10;
	hide_notifications_toggle_.setBounds(X_MARGIN, y, getWidth() - X_MARGIN, TOGGLE_BUTTON_HEIGHT);

    y = hide_notifications_toggle_.getBottom() + 10;
	hotkeys_toggle_.setBounds(X_MARGIN, y, getWidth()-X_MARGIN, TOGGLE_BUTTON_HEIGHT);

	y = hotkeys_toggle_.getBottom() + 5;
	for (auto hotkey_label : hotkey_labels_)
	{
		hotkey_label->setBounds(HOTKEY_LABEL_X, y, getWidth()-HOTKEY_LABEL_X, HOTKEY_LABEL_HEIGHT);
		y += HOTKEY_LABEL_HEIGHT + 10;
	}
}

void FxSettingsDialog::GeneralSettingsPane::paint(Graphics& g)
{
	g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

    setText();

	SettingsPane::paint(g);    
}

void FxSettingsDialog::GeneralSettingsPane::setText()
{
    auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());

    int height = FxSettingsDialog::SettingsComponent::HEIGHT;
    launch_toggle_.setButtonText(TRANS("Launch on system startup"));
    hide_help_tips_toggle_.setButtonText(TRANS("Hide help tips for audio controls"));
	hide_notifications_toggle_.setButtonText(TRANS("Hide notifications"));

    hotkeys_toggle_.setButtonText(TRANS("Disable keyboard shortcuts"));
}

FxSettingsDialog::HelpSettingsPane::HelpSettingsPane() : SettingsPane("Help"), auto_updates_toggle_(TRANS("Automatic updates"))
{	
	version_title_.setColour(Label::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	version_title_.setJustificationType(Justification::centredLeft);
	version_text_.setJustificationType(Justification::centredLeft);	
	support_title_.setColour(Label::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	support_title_.setJustificationType(Justification::centredLeft);	
	maintenance_title_.setColour(Label::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	maintenance_title_.setJustificationType(Justification::centredLeft);

	changelog_link_.setURL(URL(L"https://www.fxsound.com/changelog"));
	changelog_link_.setJustificationType(Justification::topLeft);
	quicktour_link_.setJustificationType(Justification::topLeft);
	submitlogs_link_.setJustificationType(Justification::topLeft);
	helpcenter_link_.setURL(URL(L"https://www.fxsound.com/learning-center"));
	helpcenter_link_.setJustificationType(Justification::topLeft);
    feedback_link_.setURL(URL("https://james722808.typeform.com/to/QfEP5QrP"));
	feedback_link_.setJustificationType(Justification::topLeft);

	auto_updates_toggle_.setMouseCursor(MouseCursor::PointingHandCursor);
	auto_updates_toggle_.setToggleState(FxController::getInstance().getAutoUpdates(), NotificationType::dontSendNotification);
	auto_updates_toggle_.setColour(ToggleButton::ColourIds::tickColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	auto_updates_toggle_.setColour(ToggleButton::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));

	auto_updates_toggle_.onClick = [this]() {
		FxController::getInstance().setAutoUpdates(auto_updates_toggle_.getToggleState());
	};

    setText();

	addAndMakeVisible(version_title_);
	addAndMakeVisible(version_text_);
	addAndMakeVisible(support_title_);
	addAndMakeVisible(maintenance_title_);
	addAndMakeVisible(changelog_link_);
	addChildComponent(quicktour_link_);
	addChildComponent(submitlogs_link_);
	addAndMakeVisible(helpcenter_link_);
	addAndMakeVisible(auto_updates_toggle_);
}

void FxSettingsDialog::HelpSettingsPane::resized()
{
	auto bounds = getLocalBounds().withLeft(X_MARGIN).withTop(Y_MARGIN).withHeight(TITLE_HEIGHT);
	title_.setBounds(bounds);

	version_title_.setBounds(X_MARGIN, TEXT_Y, getWidth()-X_MARGIN, TITLE_HEIGHT);
	version_text_.setBounds(X_MARGIN, version_title_.getBottom()+10, getWidth()-X_MARGIN, TEXT_HEIGHT);
	changelog_link_.setBounds(X_MARGIN+5, version_text_.getBottom()+10, getWidth()-X_MARGIN, HYPERLINK_HEIGHT);
	support_title_.setBounds(X_MARGIN, changelog_link_.getBottom()+20, getWidth()-X_MARGIN, TITLE_HEIGHT);
	helpcenter_link_.setBounds(X_MARGIN+5, support_title_.getBottom()+10, getWidth()-X_MARGIN, HYPERLINK_HEIGHT);
	maintenance_title_.setBounds(X_MARGIN, helpcenter_link_.getBottom()+20, getWidth()-X_MARGIN, TITLE_HEIGHT);
	auto_updates_toggle_.setBounds(X_MARGIN + 5, maintenance_title_.getBottom() + 10, BUTTON_WIDTH, TOGGLE_BUTTON_HEIGHT);
}

void FxSettingsDialog::HelpSettingsPane::paint(Graphics& g)
{
	g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

    setText();

	SettingsPane::paint(g);
}

void FxSettingsDialog::HelpSettingsPane::setText()
{
    auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());

    version_title_.setText(TRANS("Version"), NotificationType::dontSendNotification);
    version_title_.setFont(theme.getNormalFont());
    
    version_text_.setText(L"v" + JUCEApplication::getInstance()->getApplicationVersion(), NotificationType::dontSendNotification);
    version_text_.setFont(theme.getSmallFont());
    
    support_title_.setText(TRANS("Support"), NotificationType::dontSendNotification);
    support_title_.setFont(theme.getNormalFont());
    
    maintenance_title_.setText(TRANS("Maintenance"), NotificationType::dontSendNotification);
    maintenance_title_.setFont(theme.getNormalFont());

    changelog_link_.setButtonText(TRANS("Changelog"));    
    quicktour_link_.setButtonText(TRANS("Quick tour"));    
    submitlogs_link_.setButtonText(TRANS("Submit debug logs"));    
    helpcenter_link_.setButtonText(TRANS("Help center"));        
    feedback_link_.setButtonText(TRANS("Feedback"));
	auto_updates_toggle_.setButtonText(TRANS("Automatic updates"));;
}

FxSettingsDialog::ChannelsSettingsPane::ChannelsSettingsPane() :
	SettingsPane("Channels"),
	add_button_(TRANS("Add FxSound device"))
{
	setFocusContainer(true);

	info_.setColour(Label::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	info_.setJustificationType(Justification::topLeft);

	viewport_.setViewedComponent(&channels_, false);
	viewport_.setScrollBarsShown(true, false);

	add_button_.setMouseCursor(MouseCursor::PointingHandCursor);
	add_button_.onClick = []() {
		// The new device shows up in the list when Windows reports it, see modelChanged()
		if (!FxController::getInstance().addFxSoundDevice())
		{
			AlertWindow::showMessageBoxAsync(AlertWindow::AlertIconType::WarningIcon, TRANS("Add FxSound device"), TRANS("The FxSound device could not be added."));
		}
	};

	setText();

	addAndMakeVisible(&info_);
	addAndMakeVisible(&viewport_);
	addAndMakeVisible(&add_button_);

	FxModel::getModel().addListener(this);
}

FxSettingsDialog::ChannelsSettingsPane::~ChannelsSettingsPane()
{
	FxModel::getModel().removeListener(this);
}

void FxSettingsDialog::ChannelsSettingsPane::update()
{
	auto& controller = FxController::getInstance();
	auto& model = FxModel::getModel();
	auto outputs = controller.getActiveOutputDevices();

	names_.clear();
	outputs_.clear();
	presets_.clear();

	for (auto& fx_device : controller.getExtraFxSoundDevices())
	{
		String fx_device_id = fx_device.pwszID.c_str();
		auto route_output = controller.getRouteOutput(fx_device_id);
		auto route_preset = controller.getRoutePreset(fx_device_id);

		auto* name = names_.add(new Label({}, fx_device.deviceFriendlyName.c_str()));
		name->setColour(Label::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
		name->setTooltip(fx_device.deviceFriendlyName.c_str());

		// Item id 1 is "Off", item id i + 2 is outputs[i]
		auto* output = outputs_.add(new FxComboBox());
		output->addItem(TRANS("Off"), 1);
		output->setSelectedId(1, NotificationType::dontSendNotification);
		for (int i = 0; i < (int)outputs.size(); i++)
		{
			output->addItem(outputs[i].deviceFriendlyName.c_str(), i + 2);
			if (route_output == outputs[i].pwszID.c_str())
				output->setSelectedId(i + 2, NotificationType::dontSendNotification);
		}
		// Keeps the route to an output that isn't connected right now
		if (route_output.isNotEmpty() && output->getSelectedId() == 1)
		{
			output->addItem(TRANS("Output not connected"), DISCONNECTED_OUTPUT_ID);
			output->setSelectedId(DISCONNECTED_OUTPUT_ID, NotificationType::dontSendNotification);
		}

		// Item id 1 is the main window's preset, item id i + 2 is preset i
		auto* preset = presets_.add(new FxComboBox());
		preset->addItem(TRANS("Same as main window"), 1);
		preset->setSelectedId(1, NotificationType::dontSendNotification);
		for (int i = 0; i < model.getPresetCount(); i++)
		{
			auto preset_name = model.getPreset(i).name;
			preset->addItem(preset_name, i + 2);
			if (preset_name == route_preset)
				preset->setSelectedId(i + 2, NotificationType::dontSendNotification);
		}
		preset->setEnabled(output->getSelectedId() != 1);

		auto apply = [output, preset, fx_device_id, route_output, outputs]() {
			auto output_id = output->getSelectedId();
			String output_device_id = output_id == DISCONNECTED_OUTPUT_ID ? route_output :
				output_id >= 2 ? String(outputs[output_id - 2].pwszID.c_str()) : String();
			preset->setEnabled(output_device_id.isNotEmpty());
			FxController::getInstance().setRoute(fx_device_id, output_device_id, preset->getSelectedId() > 1 ? preset->getText() : String());
		};
		output->onChange = apply;
		preset->onChange = apply;

		channels_.addAndMakeVisible(name);
		channels_.addAndMakeVisible(output);
		channels_.addAndMakeVisible(preset);
	}

	resized();
}

void FxSettingsDialog::ChannelsSettingsPane::resized()
{
	auto bounds = getLocalBounds().withLeft(X_MARGIN).withTop(Y_MARGIN).withHeight(TITLE_HEIGHT);
	title_.setBounds(bounds);

	auto width = getWidth() - ((X_MARGIN + 5) * 2);
	info_.setBounds(X_MARGIN, INFO_Y, width, INFO_HEIGHT);
	add_button_.setBounds(X_MARGIN, getHeight() - BUTTON_HEIGHT - 20, add_button_.getBestWidthForHeight(BUTTON_HEIGHT), BUTTON_HEIGHT);
	viewport_.setBounds(X_MARGIN, info_.getBottom() + 10, width, add_button_.getY() - info_.getBottom() - 20);

	auto channel_width = viewport_.getMaximumVisibleWidth();
	channels_.setSize(channel_width, names_.size() * CHANNEL_HEIGHT);
	for (int i = 0; i < names_.size(); i++)
	{
		auto y = i * CHANNEL_HEIGHT;
		names_[i]->setBounds(0, y, channel_width, NAME_HEIGHT);
		outputs_[i]->setBounds(0, y + NAME_HEIGHT + 2, channel_width / 2 - 5, COMBO_HEIGHT);
		presets_[i]->setBounds(channel_width / 2 + 5, y + NAME_HEIGHT + 2, channel_width / 2 - 5, COMBO_HEIGHT);
	}
}

void FxSettingsDialog::ChannelsSettingsPane::paint(Graphics& g)
{
	g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

	setText();

	SettingsPane::paint(g);
}

void FxSettingsDialog::ChannelsSettingsPane::setText()
{
	auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());

	info_.setFont(theme.getNormalFont());
	info_.setText(TRANS("In Windows sound settings (App volume and device preferences), send apps to an extra FxSound device. "
		"Each one plays to its own output with its own preset. A channel follows your edits live while the main window has its preset selected."),
		NotificationType::dontSendNotification);
	add_button_.setButtonText(TRANS("Add FxSound device"));
}

void FxSettingsDialog::ChannelsSettingsPane::modelChanged(FxModel::Event model_event)
{
	if (model_event == FxModel::Event::OutputListUpdated || model_event == FxModel::Event::PresetListUpdated)
	{
		// Device changes can be reported on an audio thread
		Component::SafePointer<ChannelsSettingsPane> pane(this);
		MessageManager::callAsync([pane]() {
			if (pane != nullptr)
				pane->update();
		});
	}
}

void FxSettingsDialog::ChannelsSettingsPane::visibilityChanged()
{
	if (isVisible())
		update();
}
