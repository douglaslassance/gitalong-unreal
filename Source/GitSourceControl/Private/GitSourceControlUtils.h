// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GitSourceControlRevision.h"

class FGitSourceControlState;

class FGitSourceControlCommand;

/**
 * Helper struct for maintaining temporary files for passing to commands
 */
class FGitScopedTempFile
{
public:

	/** Constructor - open & write string to temp file */
	FGitScopedTempFile(const FText& InText);

	/** Destructor - delete temp file */
	~FGitScopedTempFile();

	/** Get the filename of this temp file - empty if it failed to be created */
	const FString& GetFilename() const;

private:
	/** The filename we are writing to */
	FString Filename;
};

struct FGitVersion;

namespace GitSourceControlUtils
{

/**
 * Find the path to the Git binary, looking into a few places (standalone Git install, and other common tools embedding Git)
 * @returns the path to the Git binary if found, or an empty string.
 */
FString FindGitBinaryPath();

/**
 * Find the path to the Gitalong binary, looking into a few places (standalone Gitalong install, and other common tools embedding Git)
 * @returns the path to the Gitalong binary if found, or an empty string.
 */
FString FindGitalongBinaryPath();

/**
 * Run a Git "version" command to check the availability of the binary.
 * @param InPathToBinary		The path to the Git binary
 * @param OutVersion         If provided, populate with the git version parsed from "version" command
 * @returns true if the command succeeded and returned no errors
 */
bool CheckGitAvailability(const FString& InPathToBinary, FGitVersion* OutVersion = nullptr);

/**
 * Run a Gitalong "version" command to check the availability of the binary.
 * @param InPathToBinary		The path to the Gitalong binary
 * @param OutVersion         If provided, populate with the git version parsed from "version" command
 * @returns true if the command succeeded and returned no errors
 */
bool CheckGitalongAvailability(const FString& InPathToBinary, FGitVersion* OutVersion = nullptr);

/**
 * Parse the output from the "version" command into GitMajorVersion and GitMinorVersion.
 * @param InVersionString       The version string returned by `git --version`
 * @param OutVersion            The FGitVersion to populate
 */
 void ParseGitVersion(const FString& InVersionString, FGitVersion* OutVersion);

/** 
 * Check git for various optional capabilities by various means.
 * @param InPathToGitBinary		The path to the Git binary
 * @param OutGitVersion			If provided, populate with the git version parsed from "version" command
 */
void FindGitCapabilities(const FString& InPathToGitBinary, FGitVersion *OutVersion);

/**
 * Run a Git "lfs" command to check the availability of the "Large File System" extension.
 * @param InPathToGitBinary		The path to the Git binary
 * @param OutGitVersion			If provided, populate with the git version parsed from "version" command
 */
 void FindGitLfsCapabilities(const FString& InPathToGitBinary, FGitVersion *OutVersion);

/**
 * Find the root of the Git repository, looking from the provided path and upward in its parent directories
 * @param InPath				The path to the Game Directory (or any path or file in any git repository)
 * @param OutRepositoryRoot		The path to the root directory of the Git repository if found, else the path to the ProjectDir
 * @returns true if the command succeeded and returned no errors
 */
bool FindRootDirectory(const FString& InPath, FString& OutRepositoryRoot);

/**
 * Get Git config user.name & user.email
 * @param	InPathToGitBinary	The path to the Git binary
 * @param	InRepositoryRoot	The Git repository from where to run the command - usually the Game directory (can be empty)
 * @param	OutUserName			Name of the Git user configured for this repository (or globaly)
 * @param	OutEmailName		E-mail of the Git user configured for this repository (or globaly)
 */
void GetUserConfig(const FString& InPathToGitBinary, const FString& InRepositoryRoot, FString& OutUserName, FString& OutUserEmail);

/**
 * Get Git current checked-out branch
 * @param	InPathToGitBinary	The path to the Git binary
 * @param	InRepositoryRoot	The Git repository from where to run the command - usually the Game directory
 * @param	OutBranchName		Name of the current checked-out branch (if any, ie. not in detached HEAD)
 * @returns true if the command succeeded and returned no errors
 */
bool GetBranchName(const FString& InPathToGitBinary, const FString& InRepositoryRoot, FString& OutBranchName);

/**
 * Get the URL of the "origin" defaut remote server
 * @param	InPathToGitBinary	The path to the Git binary
 * @param	InRepositoryRoot	The Git repository from where to run the command - usually the Game directory
 * @param	OutRemoteUrl		URL of "origin" defaut remote server
 * @returns true if the command succeeded and returned no errors
 */
bool GetRemoteUrl(const FString& InPathToGitBinary, const FString& InRepositoryRoot, FString& OutRemoteUrl);

/**
 * Run a Git command - output is a string TArray.
 *
 * @param	InCommand			The Git command - e.g. commit
 * @param	InPathToBinary		The path to the Git or Gitalong binary
 * @param	InRepositoryRoot	The Git repository from where to run the command - usually the Game directory (can be empty)
 * @param	InParameters		The parameters to the Git command
 * @param	InFiles				The files to be operated on
 * @param	OutResults			The results (from StdOut) as an array per-line
 * @param	OutErrorMessages	Any errors (from StdErr) as an array per-line
 * @returns true if the command succeeded and returned no errors
 */
bool RunCommand(const FString& InCommand, const FString& InPathToBinary, const FString& InRepositoryRoot, const TArray<FString>& InParameters, const TArray<FString>& InFiles, TArray<FString>& OutResults, TArray<FString>& OutErrorMessages);

/**
 * Run a Git "commit" command by batches.
 *
 * @param	InPathToGitBinary	The path to the Git binary
 * @param	InRepositoryRoot	The Git repository from where to run the command - usually the Game directory
 * @param	InParameter			The parameters to the Git commit command
 * @param	InFiles				The files to be operated on
 * @param	OutErrorMessages	Any errors (from StdErr) as an array per-line
 * @returns true if the command succeeded and returned no errors
 */
bool RunCommit(const FString& InPathToGitBinary, const FString& InRepositoryRoot, const TArray<FString>& InParameters, const TArray<FString>& InFiles, TArray<FString>& OutResults, TArray<FString>& OutErrorMessages);

/**
 * Run a Gitalong "clain" command by batches.
 *
 * @param	InPathToGitalongBinary	The path to the Gitalong binary
 * @param	InRepositoryRoot	The Git repository from where to run the command - usually the Game directory
 * @param	InParameter			The parameters to the Git claim command
 * @param	InFiles				The files to be operated on
 * @param	OutErrorMessages	Any errors (from StdErr) as an array per-line
 * @returns true if the command succeeded and returned no errors
 */
bool RunClaim(const FString& InPathToGitalongBinary, const FString& InRepositoryRoot, const TArray<FString>& InParameters, const TArray<FString>& InFiles, TArray<FString>& OutResults, TArray<FString>& OutErrorMessages);
	
/**
 * Run a Git "status" command and parse it.
 *
 * @param	InPathToGitBinary		The path to the Git binary
 * @param	InPathToGitalongBinary	The path to the Gitalong binary
 * @param	InRepositoryRoot		The Git repository from where to run the command - usually the Game directory (can be empty)
 * @param	InFiles					The files to be operated on
 * @param	OutErrorMessages		Any errors (from StdErr) as an array per-line
 * @returns true if the command succeeded and returned no errors
 */
bool RunUpdateStatus(const FString& InPathToGitBinary, const FString& InPathToGitalongBinary, const FString& InRepositoryRoot, const TArray<FString>& InFiles, TArray<FString>& OutErrorMessages, TArray<FGitSourceControlState>& OutStates);

/**
 * Run a Git "cat-file" command to dump the binary content of a revision into a file.
 *
 * @param	InPathToGitBinary	The path to the Git binary
 * @param	InRepositoryRoot	The Git repository from where to run the command - usually the Game directory
 * @param	InParameter			The parameters to the Git show command (rev:path)
 * @param	InDumpFileName		The temporary file to dump the revision
 * @returns true if the command succeeded and returned no errors
*/
bool RunDumpToFile(const FString& InPathToGitBinary, const FString& InRepositoryRoot, const FString& InParameter, const FString& InDumpFileName);

/**
 * Run a Git "log" command and parse it.
 *
 * @param	InPathToGitBinary	The path to the Git binary
 * @param	InRepositoryRoot	The Git repository from where to run the command - usually the Game directory
 * @param	InFile				The file to be operated on
 * @param	bMergeConflict		In case of a merge conflict, we also need to get the tip of the "remote branch" (MERGE_HEAD) before the log of the "current branch" (HEAD)
 * @param	OutErrorMessages	Any errors (from StdErr) as an array per-line
 * @param	OutHistory			The history of the file
 */
bool RunGetHistory(const FString& InPathToGitBinary, const FString& InRepositoryRoot, const FString& InFile, bool bMergeConflict, TArray<FString>& OutErrorMessages, TGitSourceControlHistory& OutHistory);

/**
 * Helper function for various commands to update cached states.
 * @returns true if any states were updated
 */
bool UpdateCachedStates(const TArray<FGitSourceControlState>& InStates);

/** 
 * Remove redundant errors (that contain a particular string) and also
 * update the commands success status if all errors were removed.
 */
void RemoveRedundantErrors(FGitSourceControlCommand& InCommand, const FString& InFilter);

}
