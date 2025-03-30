#include "cbase.h"

#if 1
#include "mp3player.h"
#include "KeyValues.h"
#include "filesystem.h"

#include "vgui_controls/MenuButton.h"
#include "vgui_controls/Menu.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/CheckButton.h"
#include "vgui_controls/Slider.h"
#include "vgui_controls/ListPanel.h"
#include "vgui/IPanel.h"
#include "vgui/IVGui.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "vgui/ILocalize.h"
#include "vgui_controls/PHandle.h"

#include "vgui_controls/PropertySheet.h"
#include "vgui_controls/PropertyPage.h"
#include "vgui_controls/TreeView.h"
#include "vgui_controls/FileOpenDialog.h"
#include "vgui_controls/DirectorySelectDialog.h"
#include "checksum_crc.h"

#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CMP3Player::CMP3Player(VPANEL parent, char const *panelName) :
	BaseClass(NULL, panelName),
	m_SelectionFrom(SONG_FROM_UNKNOWN),
	m_bDirty(false),
	m_bSettingsDirty(false),
	m_PlayListFileName(UTL_INVAL_SYMBOL),
	m_bSavingFile(false),
	m_bEnableAutoAdvance(true)
{
	g_pPlayer = this;

	// Get strings...
	g_pVGuiLocalize->AddFile("resource/mp3player_%language%.txt");
	SetParent(parent);

	SetMoveable(true);
	SetSizeable(true);
	SetMenuButtonVisible(false);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(true);

	m_pOptions = new MenuButton(this, "Menu", "#MP3Menu");

	Menu *options = new Menu(m_pOptions, "Options");
	options->AddMenuItem("AddDir", "#AddDirectory", new KeyValues("Command", "command", "adddirectory"), this);
	options->AddMenuItem("AddGame", "#AddGameSongs", new KeyValues("Command", "command", "addgamesongs"), this);
	options->AddMenuItem("Refresh", "#RefreshDb", new KeyValues("Command", "command", "refresh"), this);

	m_pOptions->SetMenu(options);

	m_pTree = new CMP3TreeControl(this, "Tree");
	m_pTree->MakeReadyForUse();

	// Make tree use small font
	IScheme *pscheme = scheme()->GetIScheme(GetScheme());
	HFont treeFont = pscheme->GetFont("DefaultVerySmall");
	m_pTree->SetFont(treeFont);

	m_pFileSheet = new CMP3FileSheet(this, "FileSheet");

	m_pPlay = new Button(this, "Play", "#Play", this, "play");
	m_pStop = new Button(this, "Stop", "#Stop", this, "stop");
	m_pNext = new Button(this, "NextTrack", "#Next", this, "nexttrack");
	m_pPrev = new Button(this, "PrevTrack", "#Prev", this, "prevtrack");
	m_pMute = new CheckButton(this, "Mute", "#Mute");
	m_pShuffle = new CheckButton(this, "Shuffle", "#Shuffle");

	m_pVolume = new Slider(this, "Volume");
	m_pVolume->SetRange((int)(MUTED_VOLUME * 100.0f), 100);
	m_pVolume->SetValue(100);

	m_pCurrentSong = new Label(this, "SongName", "#NoSong");
	m_pDuration = new Label(this, "SongDuration", "");

	m_pSongProgress = new CMP3SongProgress(this, "Progress");
	m_pSongProgress->AddActionSignalTarget(this);

	SetSize(400, 450);

	SetMinimumSize(350, 400);

	SetTitle("#MP3PlayerTitle", true);

	LoadControlSettings("resource/MP3Player.res");

	m_pCurrentSong->SetText("#NoSong");
	m_pDuration->SetText("");

	m_nCurrentFile = -1;
	m_bFirstTime = true;
	m_bPlaying = false;
	m_SongStart = -1.0f;
	m_nSongGuid = 0;
	m_nCurrentSong = 0;
	m_nCurrentPlaylistSong = 0;
	m_flCurrentVolume = 1.0f;
	m_bMuted = false;

	vgui::ivgui()->AddTickSignal(GetVPanel(), 100);

	// Automatically play a specific song
	const char* specificSong = "ui/gamestartup.mp3";
	int songIndex = FindSong(specificSong);
	if (songIndex == -1)
	{
		// If the song is not found, add it to the directory tree
		songIndex = AddSong(specificSong, 0); // Assuming 0 is the default directory index
	}

	if (songIndex != -1)
	{
		PlaySong(songIndex);
	}
	else
	{
		Warning("Unable to play the specified song: %s\n", specificSong);
	}
}
