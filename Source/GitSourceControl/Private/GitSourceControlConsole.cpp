// Copyright (c) 2014-2022 Sebastien Rombauts (sebastien.rombauts@gmail.com)
// Copyright (c) 2021-2026 Douglas Lassance

#include "GitSourceControlConsole.h"

#include "ISourceControlModule.h"

#include "GitSourceControlModule.h"
#include "GitSourceControlUtils.h"

void FGitSourceControlConsole::Register()
{
	if (!GitConsoleCommand.IsValid())
	{
		GitConsoleCommand = MakeUnique<FAutoConsoleCommand>(
			TEXT("git"),
			TEXT("Git Command Line Interface.\n")
			TEXT("Run any 'git' command directly from the Unreal Editor Console.\n")
			TEXT("Type 'git help' to get a list of commands."),
			FConsoleCommandWithArgsDelegate::CreateRaw(this, &FGitSourceControlConsole::ExecuteGitConsoleCommand)
		);
	}
	if (!GitalongConsoleCommand.IsValid())
	{
		GitalongConsoleCommand = MakeUnique<FAutoConsoleCommand>(
			TEXT("gitalong"),
			TEXT("Gitalong Command Line Interface.\n")
			TEXT("Run any 'gitalong' command directly from the Unreal Editor Console.\n")
			TEXT("Type 'gitalong help' to get a list of commands."),
			FConsoleCommandWithArgsDelegate::CreateRaw(this, &FGitSourceControlConsole::ExecuteGitalongConsoleCommand)
		);
	}
}

void FGitSourceControlConsole::Unregister()
{
	GitConsoleCommand.Reset();
	GitalongConsoleCommand.Reset();
}

void FGitSourceControlConsole::ExecuteGitConsoleCommand(const TArray<FString>& a_args)
{
	FGitSourceControlModule& GitSourceControl = FModuleManager::LoadModuleChecked<FGitSourceControlModule>("GitSourceControl");
	const FString& PathToGitBinary = GitSourceControl.AccessSettings().GetBinaryPath();
	const FString& RepositoryRoot = GitSourceControl.GetProvider().GetPathToRepositoryRoot();

	// The first argument is the command to send to git, the following ones are forwarded as parameters for the command
	TArray<FString> Parameters = a_args;
	FString Command;
	if (a_args.Num() > 0)
	{
		Command = a_args[0];
		Parameters.RemoveAt(0);
	}
	else
	{
		// If no command is provided, use "help" to emulate the behavior of the git CLI
		Command = TEXT("help");
	}

	FString Results;
	FString Errors;
	GitSourceControlUtils::RunCommandInternalRaw(Command, PathToGitBinary, RepositoryRoot, Parameters, TArray<FString>(), Results, Errors);

	UE_LOG(LogSourceControl, Log, TEXT("Output:\n%s"), *Results);
}

void FGitSourceControlConsole::ExecuteGitalongConsoleCommand(const TArray<FString>& a_args)
{
	FGitSourceControlModule& GitSourceControl = FModuleManager::LoadModuleChecked<FGitSourceControlModule>("GitSourceControl");
	const FString& PathToGitalongBinary = GitSourceControl.AccessSettings().GetGitalongBinaryPath();
	const FString& RepositoryRoot = GitSourceControl.GetProvider().GetPathToRepositoryRoot();

	// The first argument is the command to send to gitalong, the following ones are forwarded as parameters for the command
	TArray<FString> Parameters = a_args;
	FString Command;
	if (a_args.Num() > 0)
	{
		Command = a_args[0];
		Parameters.RemoveAt(0);
	}
	else
	{
		// If no command is provided, use "help" to emulate the behavior of the gitalong CLI
		Command = TEXT("help");
	}

	FString Results;
	FString Errors;
	GitSourceControlUtils::RunCommandInternalRaw(Command, PathToGitalongBinary, RepositoryRoot, Parameters, TArray<FString>(), Results, Errors);

	UE_LOG(LogSourceControl, Log, TEXT("Output:\n%s"), *Results);
}
