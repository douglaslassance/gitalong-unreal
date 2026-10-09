// Copyright (c) 2014-2022 Sebastien Rombauts (sebastien.rombauts@gmail.com)
// Copyright (c) 2021-2026 Douglas Lassance
//
// Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
// or copy at http://opensource.org/licenses/MIT)

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "GitSourceControlSettings.h"
#include "GitSourceControlProvider.h"

/**

Gitalong revision control plugin for Unreal Engine

Based on UEGitPlugin, written and contributed by Sebastien Rombauts (sebastien.rombauts@gmail.com)

Gitalong (https://github.com/douglaslassance/gitalong) shares what every clone of a repository is changing,
so that files changed elsewhere (uncommitted, unpushed, or on other branches) show as checked out by others.

### Supported features
- initialize a new Git local repository ('git init') to manage your Unreal Engine Game Project
  - can also create an appropriate .gitignore file as part of initialization
  - can also create a .gitattributes file to enable Git LFS (Large File System) as part of initialization
  - can also make the initial commit, with custom multi-line message
  - can also configure the default remote origin URL
- display status icons to show modified/added/deleted/untracked files
- display files claimed or changed by other clones, and files changed on the remote that need a sync
- check out files with 'gitalong claim', blocked when someone else is changing them
- show history of a file
- visual diff of a blueprint against depot or between previous versions of a file
- revert modifications of a file, releasing its claim
- add, delete, rename a file
- checkin/commit a file (cannot handle atomically more than 50 files)
- migrate an asset between two projects if both are using Git
- solve a merge conflict on a blueprint
- show current branch name in status text
- Sync to Pull (rebase) the current branch
- Push local commits, releasing their files for the rest of the team
- Windows, Mac and Linux

### What *cannot* be done presently
- Branch/Merge are not in the current Editor workflow
- Fetch is not in the current Editor workflow
- Amend a commit is not in the current Editor workflow
- Configure user name & email ('git config user.name' & git config user.email')

### Known issues
- the Editor does not show deleted files (only when deleted externally?)
- the Editor does not show missing files
- missing localization for git specific messages
- displaying states of 'Engine' assets (also needs management of 'out of tree' files)
- renaming a Blueprint in Editor leaves a redirector file, AND modify too much the asset to enable git to track its history through renaming

 */
class FGitSourceControlModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** Access the Git source control settings */
	FGitSourceControlSettings& AccessSettings()
	{
		return GitSourceControlSettings;
	}

	const FGitSourceControlSettings& AccessSettings() const
	{
		return GitSourceControlSettings;
	}

	/** Save the Git source control settings */
	void SaveSettings();

	/** Access the Git source control provider */
	FGitSourceControlProvider& GetProvider()
	{
		return GitSourceControlProvider;
	}

	const FGitSourceControlProvider& GetProvider() const
	{
		return GitSourceControlProvider;
	}

private:
	/** The Git source control provider */
	FGitSourceControlProvider GitSourceControlProvider;

	/** The settings for Git source control */
	FGitSourceControlSettings GitSourceControlSettings;
};
