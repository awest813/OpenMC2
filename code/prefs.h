//===========================================================================//
// Copyright (C) Microsoft Corporation. All rights reserved.                 //
//===========================================================================//

#ifndef PREFS_H
#define PREFS_H

#ifndef MCLIB_H
#include"mclib.h"
#endif


// options.cfg and orgprefs.cfg are kept in the user data directory (next to the save games), not in
// the install directory, which may be read-only or shared. initUserPrefsPaths() sets these up and
// copies the files over from the install directory the first time.
extern char UserPrefsDir[];			// ends with a path separator
extern char UserOptionsPath[];		// options.cfg
extern char UserOrgPrefsPath[];		// orgprefs.cfg
void initUserPrefsPaths();

class CPrefs {
public:
	CPrefs();
	int load( const char* pFileName = "options");
	int save();
	int applyPrefs(bool bApplyResolution = 1);

	void setNewName( const char* pNewName );
	void setNewUnit( const char* pNewUnit );
	void setNewIP( const char* pNewIP );

public:
	long DigitalMasterVolume;
	long MusicVolume;
	long sfxVolume;
	long RadioVolume;
	long BettyVolume;

	bool useShadows;
	bool useWaterInterestTexture;
	bool useHighObjectDetail;

	long GameDifficulty;
	bool useUnlimitedAmmo;

	long renderer;
	//long resolution;
    int resolutionX;
    int resolutionY;
    int displayNumber;

	bool fullScreen;
	long gammaLevel;
	bool useLeftRightMouseProfile; // if false, use old style commands
	long baseColor;
	long highlightColor;
	long faction;
	char insigniaFile[256];
	char unitName[10][256];
	char playerName[10][256];
	char ipAddresses[10][24];

	bool	pilotVideos;
	bool	useNonWeaponEffects;
	bool	useLocalShadows;
	bool	asyncMouse;
	long	fogPos;
	char	bitDepth; // 0 == 16, 1 == 32

	bool	saveTranscripts;
	bool	tutorials;

	// display (options.cfg only, no UI yet)
	bool	stretchToFit;	// stretch the game's screen to the window instead of keeping its aspect ratio
	long	vsync;			// 0 = off, 1 = on, -1 = adaptive
	long	maxFrameRate;	// frame rate cap, 0 = none

#if 0
	long FilterState;
	long TERRAIN_TXM_SIZE;
	long ObjectTextureSize;
	bool	useRealLOS;
	float doubleClickThreshold;
	long dragThreshold;
	DWORD BaseVertexColor;		//This color is applied to all vertices in game as Brightness correction.
#endif
};

extern CPrefs prefs;
#endif /*PREFS_H*/
