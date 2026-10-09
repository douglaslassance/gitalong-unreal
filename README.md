# gitalong-unreal

This is a fork of the built-in GitSourceControl [Unreal Engine](https://github.com/EpicGames/UnrealEngine) plugin written by [Sebastien Rombauts](https://github.com/SRombauts/UEGitPlugin) that integrates [Gitalong](https://github.com/douglaslassance/gitalong) mechanisms to prevent conflicts when working with a team.

## Pre-requisites

- [Unreal Engine >=5.0](https://www.unrealengine.com/download)
- [Gitalong](https://github.com/douglaslassance/gitalong/)

## Installation

The recommended way to install this plugin is to submodule this repository inside the `Plugins` folder of a code project.

```shell
git submodule add https://github.com/douglaslassance/gitalong-unreal Path/To/Plugins/Gitalong
```

Make sure your repository is set up for Gitalong by following the directions featured [here](https://github.com/douglaslassance/gitalong).

> [!IMPORTANT]
> We currently don't provide a built version of the plugin therefore non-code projects will not be able to use the plugin without extra work on your part.

## Usage

The plugin should work as you'd expect where asset being worked on by other member of your team or modified in feature branches will be "checked out" by others.
You should be able to submit/commit or revert changes using the editor or any Git interface interchangeably.
