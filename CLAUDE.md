# CLAUDE.md

C++23 (MSVC), SFML 3. Match the surrounding code.

## Style

- Tabs, CRLF line endings, final newline.
- Allman braces. A single-statement body stays on the same line without braces: `if (isFinished) return true;`.
- `.cpp` files start with `#include "stdafx.h"`. Headers use `#pragma once`.
- One class per file, named after the class. Add every new or removed file to the `.vcxproj` and `.vcxproj.filters`.
- Constants and enum values are `UPPER_SNAKE_CASE`, enums are `enum class`. Everything else is camelCase or PascalCase, with no member prefixes.
- `float` only, literals always have the `f` suffix.
- Comments are `//` lines inside function bodies only: a short capitalised phrase with no trailing period. None above classes, namespaces, members or functions, and no doc comments or commented-out code.

## Project

- `TBML/` is the library (`tbml::ga`, `tbml::nn`, `tbml::fn`), the apps are in the global namespace.
- Prefer composition over inheritance. Shared behaviour goes in components a class owns (`Camera`, `FitnessGraph`, `NNRenderer`), not in extra base classes.
- New scenario: derive from `Scenario` (the only base), hold the genepool as a member, implement `getGenepool` and `renderAgents`, then register it in `App.cpp`. If the agents have `render(window, bool isElite)`, `renderAgents` is `this->renderGenepoolAgents(window, *genepool->getAgentPopulation())`. Override `renderWorld` (world space) or `renderOverlay` (screen space) as needed. For a network viewer, add `getBrain()` / `getNetInput()` to the agent and own an `NNRenderer`.
- UI colours, sizes and spacing come from `UITheme.h`.
- Build through the solution: `MSBuild MachineLearning.sln /t:TBMLGeneticAlgorithm /p:Configuration=Release /p:Platform=x64`. Close the running exe first or linking fails.
