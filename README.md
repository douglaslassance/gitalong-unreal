# gitalong-unreal

This a replacement for the built-in GitSourceControl Unreal plugin that integrate [Gitalong]() mechanisms to prevent conflicts when working with a team.

## Pre-requisites

- [Git >=2.35.1](https://git-scm.com/downloads)
- [Gitalong >= 0.1.0.dev1](https://github.com/douglaslassance/gitalong-cli/releases)

## Installation

The recommended way to install this plugin is to submodule this repository inside your `Plugins` folder.

```shell
git submodule add https://github.com/douglaslassance/gitalong-unreal Path/To/Plugins/Gitalong
```

The plugins expects the project to be setup for Gitalong by following the directions feature on [Gitalong CLI](https://github.com/douglaslassance/gitalong-cli).

## Usage

The plugin should work as you'd expect where asset being worked by other member of your team or modified in feature branches will be "checked out by others".
You should be able to submit/commit or revert via the engine or any Git interface interchangeably.
