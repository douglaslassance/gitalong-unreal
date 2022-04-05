# gitalong-unreal

This a replacement for the built-in GitSourceControl Unreal plugin that integrate [Gitalong](https://github.com/douglaslassance/gitalong) mechanisms to prevent conflicts when working with a team.

## Pre-requisites

- [Unreal Engine >=4.26](https://www.unrealengine.com/en-US/download)
- [Git >=2.35.1](https://git-scm.com/downloads)
- [Gitalong](https://github.com/douglaslassance/gitalong-cli/)

## Installation

The recommended way to install this plugin is to submodule this repository inside your `Plugins` folder.

```shell
git submodule add https://github.com/douglaslassance/gitalong-unreal Path/To/Plugins/Gitalong
```

The plugins expects the project to be setup for Gitalong by following the directions featured on [Gitalong CLI](https://github.com/douglaslassance/gitalong-cli).

## Usage

The plugin should work as you'd expect where asset being worked by other member of your team or modified in feature branches will be "checked out by others".
You should be able to submit/commit or revert via the engine or any Git interface interchangeably.
