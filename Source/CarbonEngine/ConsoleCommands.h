/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Core/BuildInfo.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/FileSystem/FileSystemVolume.h"
#include "CarbonEngine/Core/InterfaceRegistry.h"
#include "CarbonEngine/Core/Memory/BlockAllocator.h"
#include "CarbonEngine/Core/Memory/BlockAllocatorSet.h"
#include "CarbonEngine/Core/Memory/MemoryLeakDetector.h"
#include "CarbonEngine/Core/Memory/MemoryStatistics.h"
#include "CarbonEngine/Core/Memory/MemoryValidator.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Graphics/States/CachedState.h"
#include "CarbonEngine/Graphics/States/StateCacher.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Physics/PhysicsInterface.h"
#include "CarbonEngine/Platform/Console.h"
#include "CarbonEngine/Platform/ConsoleCommand.h"
#include "CarbonEngine/Platform/FrameTimers.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Platform/ThemeManager.h"
#include "CarbonEngine/Render/EffectManager.h"
#include "CarbonEngine/Render/Font.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Render/Shaders/Shader.h"
#include "CarbonEngine/Render/Texture/Texture2D.h"
#include "CarbonEngine/Render/Texture/TextureManager.h"
#include "CarbonEngine/Scene/Camera.h"
#include "CarbonEngine/Scene/Material.h"
#include "CarbonEngine/Scene/MaterialManager.h"
#include "CarbonEngine/Scene/Scene.h"
#include "CarbonEngine/Sound/SoundInterface.h"
#include "CarbonEngine/Sound/SoundShader.h"
#include "CarbonEngine/Sound/SoundShaderManager.h"

namespace Carbon
{

static void runListConsoleCommand(const Vector<UnicodeString>& parameters)
{
    console().printInColumns(console().getRegisteredCommands(), true);
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(List, "")

static void runClearConsoleCommand(const Vector<UnicodeString>& parameters)
{
    console().clearHistory();
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(Clear, "")

static void runSizeConsoleCommand(const Vector<UnicodeString>& parameters)
{
    console().setScreenFraction(parameters[0].asFloat());
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(Size, "<screen fraction>")

static void runHistorySizeConsoleCommand(const Vector<UnicodeString>& parameters)
{
    console().setMaximumHistorySize(parameters[0].asInteger());
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(HistorySize, "<line count>")

#ifdef CARBON_INCLUDE_MEMORY_INTERCEPTOR

static void runMemoryStatisticsConsoleCommand(const Vector<UnicodeString>& parameters)
{
    MemoryStatistics::logAllocationDetails();
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(MemoryStatistics, "")

static void runMemoryStressTestConsoleCommand(const Vector<UnicodeString>& parameters)
{
    if (parameters.size())
        MemoryValidator::EnableStressTest = parameters[0].asBoolean();
    else
        MemoryValidator::EnableStressTest = !MemoryValidator::EnableStressTest;
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(MemoryStressTest, "[<on|off>]")

static void runMemoryLoggingConsoleCommand(const Vector<UnicodeString>& parameters)
{
    if (parameters.size())
        MemoryInterceptor::EnableLogging = parameters[0].asBoolean();
    else
        MemoryInterceptor::EnableLogging = !MemoryInterceptor::EnableLogging;
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(MemoryLogging, "[<on|off>]")

static void runMemoryLeakDetectorConsoleCommand(const Vector<UnicodeString>& parameters)
{
    if (MemoryLeakDetector::isEnabled())
        MemoryLeakDetector::disable();
    else
        LOG_CONSOLE << "The memory leak detector is already disabled";
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(MemoryLeakDetector, "<off>")

#endif

static void runMemorySummaryConsoleCommand(const Vector<UnicodeString>& parameters)
{
    GatherMemorySummaryEvent::report();
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(MemorySummary, "")

static void runBlockAllocatorsConsoleCommand(const Vector<UnicodeString>& parameters)
{
    auto blockAllocators = MemoryInterceptor::getBlockAllocators();

    if (blockAllocators)
        blockAllocators->printInfo();
    else
        LOG_CONSOLE << "There are no block allocators in use";
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(BlockAllocators, "")

static void runBuildInfoConsoleCommand(const Vector<UnicodeString>& parameters)
{
    console().printInColumns(U(BuildInfo::getBuildInfo()), false);
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(BuildInfo, "")

static void runExitConsoleCommand(const Vector<UnicodeString>& parameters)
{
    events().dispatchEvent(ShutdownRequestEvent());
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(Exit, "")

static void runLogEventsConsoleCommand(const Vector<UnicodeString>& parameters)
{
    if (parameters.empty())
        EventManager::LogEvents = !EventManager::LogEvents;
    else
        EventManager::LogEvents = parameters[0].asBoolean();
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(LogEvents, "[<true|false>]")

static void runReadFileConsoleCommand(const Vector<UnicodeString>& parameters)
{
    auto string = UnicodeString();

    if (!fileSystem().readTextFile(parameters[0], string))
    {
        LOG_CONSOLE << "Error reading file: " << parameters[0];
        return;
    }

    // Print lines
    for (const auto& line : string.getLines())
        LOG_CONSOLE << line;
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(ReadFile, "<filename>")

static void runWriteFileConsoleCommand(const Vector<UnicodeString>& parameters)
{
    try
    {
        // Open the file
        auto file = FileWriter();
        fileSystem().open(parameters[0], file);

        // Write the contents of the second parameter to the file
        file.writeText(parameters[1]);

        file.close();
    }
    catch (const Exception&)
    {
        LOG_CONSOLE << "Failed writing file: " << parameters[0];
    }
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(WriteFile, "<filename> <contents>")

static void runDoesFileExistConsoleCommand(const Vector<UnicodeString>& parameters)
{
    LOG_CONSOLE << (fileSystem().doesFileExist(parameters[0]) ? "Yes" : "No");
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(DoesFileExist, "<filename>")

static void runListVolumesConsoleCommand(const Vector<UnicodeString>& parameters)
{
    console().printInColumns(fileSystem().getVolumeNames(), true, 1);
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(ListVolumes, "")

static void runEnumerateFilesConsoleCommand(const Vector<UnicodeString>& parameters)
{
    // Make the extensions * and *.* list all files
    auto extension = (parameters.size() == 1 || parameters[1] == "*" || parameters[1] == "*.*") ? "" : parameters[1];

    // Enumerate the files using the given parameters
    auto files = Vector<UnicodeString>();
    fileSystem().enumerateFiles(parameters[0], extension, parameters.size() > 2 ? parameters[2].asBoolean() : true, files);

    // Print results
    if (files.empty())
        LOG_CONSOLE << "No files found";
    else
        console().printInColumns(files, true, 1);
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(EnumerateFiles, "<directory> [<extension> = *] [<recursive> = true]")

static void runFreeSpaceConsoleCommand(const Vector<UnicodeString>& parameters)
{
    auto volume = fileSystem().getVolume(parameters[0]);
    if (!volume)
    {
        LOG_CONSOLE << "Unknown volume";
        return;
    }

    LOG_CONSOLE << FileSystem::formatByteSize(volume->getFreeSpaceInBytes());
}
static void autocompleteFreeSpaceConsoleCommand(unsigned int parameter, Vector<UnicodeString>& completions)
{
    completions = fileSystem().getVolumeNames();
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND_WITH_AUTOCOMPLETE(FreeSpace, "<volume>")

static void runPhysicsInfoConsoleCommand(const Vector<UnicodeString>& parameters)
{
    LOG_CONSOLE << "Physics engine: " << physics().getEngineName();
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(PhysicsInfo, "")

static void runResolutionConsoleCommand(const Vector<UnicodeString>& parameters)
{
    for (auto& resolution : platform().getResolutions())
    {
        if (parameters[0] == resolution)
        {
            if (!platform().resizeWindow(resolution, platform().getWindowMode(), platform().getFSAAMode()))
                LOG_CONSOLE << "Error: failed changing resolution";

            return;
        }
    }

    LOG_CONSOLE << "Error: unsupported resolution '" << parameters[0] << "'";
}
static void autocompleteResolutionConsoleCommand(unsigned int parameterIndex, Vector<UnicodeString>& completions)
{
    completions = platform().getResolutions().map<UnicodeString>();
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND_WITH_AUTOCOMPLETE(Resolution, "<widthxheight>")

static void runFullscreenConsoleCommand(const Vector<UnicodeString>& parameters)
{
    if (platform().getWindowMode() != PlatformInterface::Fullscreen)
    {
        // Change the window mode
        if (!platform().resizeWindow(platform().getCurrentResolution(), PlatformInterface::Fullscreen,
                                     platform().getFSAAMode()))
            LOG_CONSOLE << "Error: failed changing window mode";
    }
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(Fullscreen, "")

static void runWindowedConsoleCommand(const Vector<UnicodeString>& parameters)
{
    if (platform().getWindowMode() != PlatformInterface::Windowed)
    {
        // Change the window mode
        if (!platform().resizeWindow(platform().getCurrentResolution(), PlatformInterface::Windowed, platform().getFSAAMode()))
            LOG_CONSOLE << "Error: failed changing window mode";
    }
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(Windowed, "")

static void runFSAAConsoleCommand(const Vector<UnicodeString>& parameters)
{
    auto fsaa = parameters[0].asInteger();

    if (fsaa != 0 && fsaa != 2 && fsaa != 4 && fsaa != 8 && fsaa != 16)
    {
        LOG_CONSOLE << "Error: invalid parameter";
        return;
    }

    if (platform().getFSAAMode() != PlatformInterface::FSAAMode(fsaa))
    {
        // Change the FSAA mode
        if (!platform().resizeWindow(platform().getCurrentResolution(), platform().getWindowMode(),
                                     PlatformInterface::FSAAMode(fsaa)))
            LOG_CONSOLE << "Error: failed changing window mode";
    }
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(FSAA, "<0|2|4|8|16>")

static void runListResolutionsConsoleCommand(const Vector<UnicodeString>& parameters)
{
    auto resolutions = Vector<UnicodeString>();

    for (auto& resolution : platform().getResolutions())
        resolutions.append(UnicodeString(resolution) + (resolution.isCustomResolution() ? " (custom)" : ""));

    console().printInColumns(resolutions, false);
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(ListResolutions, "")

static void runReleaseInputLockConsoleCommand(const Vector<UnicodeString>& parameters)
{
    platform().releaseInputLock();
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(ReleaseInputLock, "")

static void runVerticalSyncConsoleCommand(const Vector<UnicodeString>& parameters)
{
    if (!platform().setVerticalSyncEnabled(parameters.size() ? parameters[0].asBoolean() : !platform().isVerticalSyncEnabled()))
        LOG_CONSOLE << "Failed changing vertical sync";
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(VerticalSync, "[<true|false>]")

static void runGammaConsoleCommand(const Vector<UnicodeString>& parameters)
{
    platform().setGamma(parameters[0].asFloat());
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(Gamma, "<gamma>")

static void runGammaRGBConsoleCommand(const Vector<UnicodeString>& parameters)
{
    platform().setGamma(Color(parameters[0].asFloat(), parameters[1].asFloat(), parameters[2].asFloat()));
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(GammaRGB, "<red gamma> <green gamma> <blue gamma>")

static void runGUIThemeConsoleCommand(const Vector<UnicodeString>& parameters)
{
    if (!theme().load(A(parameters[0])))
        LOG_CONSOLE << "Error: failed loading theme";
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(GUITheme, "<name>")

static void runFrameTimersConsoleCommand(const Vector<UnicodeString>& parameters)
{
    if (parameters.empty())
        FrameTimers::Enabled = !FrameTimers::Enabled;
    else
        FrameTimers::Enabled = parameters[0].asBoolean();

    renderer().setFrameTimerRenderingEnabled(FrameTimers::Enabled);
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(FrameTimers, "[<true|false>]")

static void runFrameTimersVisibleConsoleCommand(const Vector<UnicodeString>& parameters)
{
    if (parameters.empty())
        renderer().setFrameTimerRenderingEnabled(!renderer().isFrameTimerRenderingEnabled());
    else
        renderer().setFrameTimerRenderingEnabled(parameters[0].asBoolean());
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(FrameTimersVisible, "[<true|false>]")

static void runGraphicsInterfaceConsoleCommand(const Vector<UnicodeString>& parameters)
{
    typedef InterfaceRegistry<GraphicsInterface> GraphicsInterfaceRegistry;

    if (parameters.size() == 1)
    {
        for (auto implementation : GraphicsInterfaceRegistry::getImplementations())
        {
            if (A(parameters[0]).asLower() == implementation->getName().asLower())
            {
                implementation->getName().copyUTF8To(GraphicsInterfaceRegistry::OverrideImplementationName,
                                                     sizeof(GraphicsInterfaceRegistry::OverrideImplementationName));

                platform().resizeWindow(platform().getCurrentResolution(), platform().getWindowMode(),
                                        platform().getFSAAMode());

                return;
            }
        }

        LOG_CONSOLE << "Error: unknown graphics interface: " << parameters[0];
    }
    else
    {
        for (auto implementation : GraphicsInterfaceRegistry::getImplementations())
        {
            LOG_CONSOLE << implementation->getName().padToLength(20)
                        << "priority: " << String(implementation->getPriority()).padToLength(20)
                        << (implementation == GraphicsInterfaceRegistry::getActiveImplementation() ? "<- Active" : "");
        }
    }
}
static void autocompleteGraphicsInterfaceConsoleCommand(unsigned int parameterIndex, Vector<UnicodeString>& completions)
{
    typedef InterfaceRegistry<GraphicsInterface> GraphicsInterfaceRegistry;

    completions = U(GraphicsInterfaceRegistry::getImplementationNames());
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND_WITH_AUTOCOMPLETE(GraphicsInterface, "[<interface name>]")

static void runCompileFontConsoleCommand(const Vector<UnicodeString>& parameters)
{
    // Read texture size, if specified
    auto textureSize = 512U;
    if (parameters.size() >= 3)
        textureSize = parameters[2].asInteger();

    // Read the characters text file, if one was specified
    auto codePoints = UnicodeString();
    if (parameters.size() >= 4 && !fileSystem().readTextFile(parameters[3], codePoints))
    {
        LOG_CONSOLE << "Failed reading the additional characters text file '" << parameters[3] << "'";
        return;
    }

    auto font = Font();
    if (font.loadFromSystemFont(parameters[0], parameters[1].asInteger(), codePoints, textureSize) && font.save())
    {
        LOG_CONSOLE << "Font compile succeeded, character count: " << font.getCharacters().size()
                    << ", native size: " << font.getMaximumCharacterHeightInPixels() << "px";
    }
    else
        LOG_CONSOLE << "Failed compiling font";
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(CompileFont, "<font name> <font size> [<texture size> = 512] [<characters text file>]")

static void runListTextureGroupsConsoleCommand(const Vector<UnicodeString>& parameters)
{
    console().printInColumns(U(textures().getTextureGroups()), true);
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(ListTextureGroups, "")

static void runListTexturesConsoleCommand(const Vector<UnicodeString>& parameters)
{
    auto textureNames = textures().getTextureNames();

    if (parameters.size())
    {
        if (parameters[0].asLower() == "all")
            ;
        else if (parameters[0].asLower() == "npot")
        {
            for (auto i = 0U; i < textureNames.size(); i++)
            {
                if (!textures().getTexture(textureNames[i])->getImage().isNPOT())
                    textureNames.erase(i--);
            }
        }
        else
            LOG_CONSOLE << "Error: invalid parameter";
    }

    console().printInColumns(U(textureNames), true);
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(ListTextures, "[<All|NPOT> = All]")

static void runTextureMemoryConsoleCommand(const Vector<UnicodeString>& parameters)
{
    auto textureNames = textures().getTextureNames();

    if (parameters.empty() || parameters[0].asLower() == "sortbyname")
    {
        // Already done
    }
    else if (parameters[0].asLower() == "sortbysize")
    {
        textureNames.sortBy([](const String& first, const String& second) {
            return textures().getTexture(first)->getImage().getDataSize() <
                textures().getTexture(second)->getImage().getDataSize();
        });
    }
    else
    {
        LOG_CONSOLE << "Error: invalid parameter";
        return;
    }

    auto maximumNameLength = String::longestString(textureNames);

    auto maximumPixelFormatNameLength = String::longestString(textureNames.map<String>([](const String& name) {
        return Image::getPixelFormatString(textures().getTexture(name)->getImage().getPixelFormat());
    }));

    auto totalImageDataSize = 0U;

    for (const auto& name : textureNames)
    {
        auto texture = textures().getTexture(name);
        auto& image = texture->getImage();

        totalImageDataSize += image.getDataSize();

        LOG_CONSOLE << name.padToLength(maximumNameLength + 4)
                    << FileSystem::formatByteSize(image.getDataSize()).prePadToLength(10)
                    << Image::getPixelFormatString(image.getPixelFormat()).prePadToLength(maximumPixelFormatNameLength + 4)
                    << (String() + image.getWidth() + "x" + image.getHeight() +
                        (image.getDepth() > 1 ? (String() + "x" + image.getDepth()) : ""))
                           .prePadToLength(13)
                    << "    " << Texture::convertTextureTypeToString(texture->getTextureType());
    }

    LOG_CONSOLE << "";
    LOG_CONSOLE << "There are " << textureNames.size() << " textures using " << FileSystem::formatByteSize(totalImageDataSize);
}
static void autocompleteTextureMemoryConsoleCommand(unsigned int parameterIndex, Vector<UnicodeString>& completions)
{
    completions = {"SortByName", "SortBySize"};
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND_WITH_AUTOCOMPLETE(TextureMemory, "[<SortByName|SortBySize = SortByName>]")

static void runLoadTexturesConsoleCommand(const Vector<UnicodeString>& parameters)
{
    textures().reloadTextures();
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(LoadTextures, "")

static void runTextureAnisotropyConsoleCommand(const Vector<UnicodeString>& parameters)
{
    auto group = String("WorldDiffuse");
    if (parameters.size() == 2)
    {
        if (textures().getTextureGroups().has(A(parameters[1])))
            group = A(parameters[1]);
        else
        {
            LOG_CONSOLE << "Error: unknown texture group";
            return;
        }
    }

    auto properties = textures().getGroupProperties(group);

    if (parameters[0] == "1")
        properties.setAnisotropy(1);
    else if (parameters[0] == "2")
        properties.setAnisotropy(2);
    else if (parameters[0] == "4")
        properties.setAnisotropy(4);
    else if (parameters[0] == "8")
        properties.setAnisotropy(8);
    else if (parameters[0] == "16")
        properties.setAnisotropy(16);
    else
        LOG_CONSOLE << "Error: invalid anisotropy value";

    textures().setGroupProperties(group, properties);
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(TextureAnisotropy, "<1|2|4|8|16> [<texture group> = WorldDiffuse]")

static void runTextureFilterConsoleCommand(const Vector<UnicodeString>& parameters)
{
    auto group = String("WorldDiffuse");
    if (parameters.size() == 2)
    {
        if (textures().getTextureGroups().has(A(parameters[1])))
            group = A(parameters[1]);
        else
        {
            LOG_CONSOLE << "Error: unknown texture group";
            return;
        }
    }

    auto properties = textures().getGroupProperties(group);

    if (parameters[0].asLower() == "nearest")
        properties.setFilter(TextureProperties::NearestFilter);
    else if (parameters[0].asLower() == "bilinear")
        properties.setFilter(TextureProperties::BilinearFilter);
    else if (parameters[0].asLower() == "trilinear")
        properties.setFilter(TextureProperties::TrilinearFilter);
    else
        LOG_CONSOLE << "Error: invalid filter";

    textures().setGroupProperties(group, properties);
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(TextureFilter, "<Nearest|Bilinear|Trilinear> [<texture group> = WorldDiffuse]")

static void runTextureQualityConsoleCommand(const Vector<UnicodeString>& parameters)
{
    auto group = String("WorldDiffuse");
    if (parameters.size() == 2)
    {
        if (textures().getTextureGroups().has(A(parameters[1])))
            group = A(parameters[1]);
        else
        {
            LOG_CONSOLE << "Error: unknown texture group";
            return;
        }
    }

    auto properties = textures().getGroupProperties(group);

    if (parameters[0].asLower() == "low")
        properties.setQuality(TextureProperties::TextureQualityLow);
    else if (parameters[0].asLower() == "medium")
        properties.setQuality(TextureProperties::TextureQualityMedium);
    else if (parameters[0].asLower() == "high")
        properties.setQuality(TextureProperties::TextureQualityHigh);
    else if (parameters[0].asLower() == "maximum")
        properties.setQuality(TextureProperties::TextureQualityMaximum);
    else if (parameters[0].isInteger())
        properties.setQuality(parameters[0].asInteger());
    else
        LOG_CONSOLE << "Error: invalid quality";

    textures().setGroupProperties(group, properties);
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(TextureQuality, "<Low|Medium|High|Maximum> [<texture group> = WorldDiffuse]")

static void runHDRConsoleCommand(const Vector<UnicodeString>& parameters)
{
    if (parameters.size() == 1)
        renderer().setHDREnabled(parameters[0].asBoolean());
    else
        renderer().setHDREnabled(!renderer().isHDREnabled());
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(HDR, "[<true|false>]")

static void runListEffectsConsoleCommand(const Vector<UnicodeString>& parameters)
{
    console().printInColumns(U(effects().getEffectNames()), true);
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(ListEffects, "")

static void runListShadersConsoleCommand(const Vector<UnicodeString>& parameters)
{
    if (parameters.size() == 0)
    {
        auto longest = String::longestString(effects().getEffectNames());

        for (auto& effect : effects().getEffects())
        {
            auto shaderName = String();
            if (effect->getActiveShader())
                shaderName = effect->getActiveShader()->getClassName();
            else
                shaderName = "--- None ---";

            LOG_CONSOLE << effect->getName().padToLength(longest + 4) << shaderName;
        }
    }
    else
    {
        auto effect = effects().getEffect(A(parameters[0]));
        if (!effect)
        {
            LOG_CONSOLE << "Unknown effect";
            return;
        }

        const auto& effectShaders = effect->getAllShaders();

        auto shaderNames = effectShaders.map<String>([](const Shader* shader) { return shader->getClassName(); });
        auto longestShaderName = String::longestString(shaderNames);

        for (auto i = 0U; i < effectShaders.size(); i++)
        {
            shaderNames[i].resize(longestShaderName + 4, ' ');
            shaderNames[i] << effectShaders[i]->getQuality();
        }

        for (auto i = 0U; i < effectShaders.size(); i++)
        {
            shaderNames[i].resize(longestShaderName + 10, ' ');

            if (effectShaders[i]->hasHardwareSupport())
                shaderNames[i] << "supported";
            else
                shaderNames[i] << "unsupported";

            if (effectShaders[i] == effect->getActiveShader())
            {
                shaderNames[i].resize(shaderNames[i].length() + 4, ' ');
                shaderNames[i] << "(active)";
            }
        }

        console().printInColumns(U(shaderNames), true);
    }
}
static void autocompleteListShadersConsoleCommand(unsigned int parameterIndex, Vector<UnicodeString>& completions)
{
    completions = U(effects().getEffectNames());
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND_WITH_AUTOCOMPLETE(ListShaders, "[<effect name>]")

static void runLoadShadersConsoleCommand(const Vector<UnicodeString>& parameters)
{
    for (auto activeShader : effects().getAllActiveShaders())
    {
        if (activeShader->isSetup())
        {
            activeShader->cleanup();
            activeShader->setup();
        }
    }
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(LoadShaders, "")

static void runPrecacheShadersConsoleCommand(const Vector<UnicodeString>& parameters)
{
    for (auto activeShader : effects().getAllActiveShaders())
    {
        activeShader->setup();
        if (activeShader->isSetup())
            activeShader->precache();
    }
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(PrecacheShaders, "")

static void runEffectQualityConsoleCommand(const Vector<UnicodeString>& parameters)
{
    auto quality = 0U;
    if (parameters[0].asLower() == "low")
        quality = Effect::LowShaderQuality;
    else if (parameters[0].asLower() == "medium")
        quality = Effect::MediumShaderQuality;
    else if (parameters[0].asLower() == "high")
        quality = Effect::HighShaderQuality;
    else if (parameters[0].asLower() == "maximum")
        quality = Effect::MaximumShaderQuality;
    else if (parameters[0].isInteger())
        quality = parameters[0].asInteger();
    else
    {
        LOG_CONSOLE << "Error: invalid quality setting";
        return;
    }

    for (auto effect : effects().getEffects())
        effects().getEffect(effect->getName())->updateActiveShader(quality);
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(EffectQuality, "<Low|Medium|High|Maximum>")

static void runShadowMapSizeConsoleCommand(const Vector<UnicodeString>& parameters)
{
    renderer().setShadowMapSize(parameters[0].asInteger());
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(ShadowMapSize, "<size>")

static void runShowDebugInfoConsoleCommand(const Vector<UnicodeString>& parameters)
{
    if (parameters.size() == 1)
        renderer().setShowDebugInfo(parameters[0].asBoolean());
    else
        renderer().setShowDebugInfo(!renderer().getShowDebugInfo());
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(ShowDebugInfo, "[<true|false>]")

static void runShowFPSConsoleCommand(const Vector<UnicodeString>& parameters)
{
    if (parameters.size() == 1)
        renderer().setShowFPS(parameters[0].asBoolean());
    else
        renderer().setShowFPS(!renderer().getShowFPS());
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(ShowFPS, "[<true|false>]")

static void runStateTraceConsoleCommand(const Vector<UnicodeString>& parameters)
{
    for (const auto& stateItem : States::StateCacher::getCurrentState())
        LOG_CONSOLE << stateItem;
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(StateTrace, "")

static void runStateUpdateCountsConsoleCommand(const Vector<UnicodeString>& parameters)
{
    auto results = Vector<String>();

    for (auto state : States::StateCacher::getCachedStates())
    {
        if (state->getGraphicsInterfaceStateUpdateCount())
            results.append(state->getName() + ": " + state->getGraphicsInterfaceStateUpdateCount());
    }

    console().printInColumns(U(results), true);
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(StateUpdateCounts, "")

static void runPrintRenderQueuesConsoleCommand(const Vector<UnicodeString>& parameters)
{
    renderer().printRenderQueues();
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(PrintRenderQueues, "")

static void runDebugTextureConsoleCommand(const Vector<UnicodeString>& parameters)
{
    auto name = (parameters.size() >= 1) ? A(parameters[0]) : String::Empty;
    auto frame = (parameters.size() >= 2) ? parameters[1].asInteger() : 0;
    auto mipmap = (parameters.size() >= 3) ? parameters[2].asInteger() : 0;
    auto renderAlpha = (parameters.size() >= 4) ? parameters[3].asBoolean() : false;
    auto scale = (parameters.size() >= 5) ? parameters[4].asFloat() : 1.0f;

    renderer().setDebugTexture(name, frame, mipmap, renderAlpha, scale);
}
static void autocompleteDebugTextureConsoleCommand(unsigned int parameterIndex, Vector<UnicodeString>& completions)
{
    if (parameterIndex == 0)
        completions = U(textures().getTextureNames());
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND_WITH_AUTOCOMPLETE(
    DebugTexture, "[<texture name>] [<frame> = 0] [<mipmap> = 0] [<render alpha channel> = false] [<scale> = 1]")

static void runSampleTextureConsoleCommand(const Vector<UnicodeString>& parameters)
{
    auto texture = dynamic_cast<Texture2D*>(textures().getTexture(A(parameters[0])));
    if (texture)
        LOG_CONSOLE << "Sample result: " << texture->sampleNearestTexel(parameters[0].asFloat(), parameters[1].asFloat());
    else
        LOG_CONSOLE << "Error: unknown 2D texture";
}
static void autocompleteSampleTextureConsoleCommand(unsigned int parameterIndex, Vector<UnicodeString>& completions)
{
    if (parameterIndex == 0)
        completions = U(textures().getTextureNames());
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND_WITH_AUTOCOMPLETE(SampleTexture, "<texture name> <u> <v>")

static void runListScenesConsoleCommand(const Vector<UnicodeString>& parameters)
{
    auto names = Scene::getAllScenes().map<UnicodeString>([](const Scene* scene) { return scene->getName(); });

    console().printInColumns(names, true);
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(ListScenes, "")

static void runPrintScenesConsoleCommand(const Vector<UnicodeString>& parameters)
{
    for (auto scene : Scene::getAllScenes())
    {
        if (parameters.empty() || scene->getName() == A(parameters[0]))
            scene->debugTrace();
    }
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(PrintScenes, "[<scene name>]")

static void runInvertMouseConsoleCommand(const Vector<UnicodeString>& parameters)
{
    if (parameters.size() == 1)
        PlayerEntityController::InvertMouse = parameters[0].asBoolean();
    else
        PlayerEntityController::InvertMouse = !PlayerEntityController::InvertMouse;
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(InvertMouse, "[<true|false>]")

static void runListMaterialsConsoleCommand(const Vector<UnicodeString>& parameters)
{
    console().printInColumns(U(materials().getMaterialNames()), true);
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(ListMaterials, "")

static void runLoadMaterialsConsoleCommand(const Vector<UnicodeString>& parameters)
{
    materials().reloadMaterials();
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(LoadMaterials, "")

static void runMaterialParametersConsoleCommand(const Vector<UnicodeString>& parameters)
{
    LOG_CONSOLE << materials().getMaterial(A(parameters[0])).getParameters();
}
static void autocompleteMaterialParametersConsoleCommand(unsigned int parameterIndex, Vector<UnicodeString>& completions)
{
    completions = U(materials().getMaterialNames());
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND_WITH_AUTOCOMPLETE(MaterialParameters, "<material name>")

static void runSetMaterialParameterConsoleCommand(const Vector<UnicodeString>& parameters)
{
    materials().getMaterial(A(parameters[0])).setParameter(A(parameters[1]), A(parameters[2]));
}
static void autocompleteSetMaterialParameterConsoleCommand(unsigned int parameterIndex, Vector<UnicodeString>& completions)
{
    if (parameterIndex == 0)
        completions = U(materials().getMaterialNames());
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND_WITH_AUTOCOMPLETE(SetMaterialParameter, "<material> <parameter> <value>")

static void runRewriteMaterialsConsoleCommand(const Vector<UnicodeString>& parameters)
{
    auto materialFiles = Vector<UnicodeString>();
    fileSystem().enumerateFiles(Material::MaterialDirectory, Material::MaterialExtension, true, materialFiles);

    auto savedMaterialCount = 0U;

    for (const auto& file : materialFiles)
    {
        auto material = Material();
        if (!material.load(file))
            continue;

        if (material.save(file))
            savedMaterialCount++;
    }

    LOG_CONSOLE << "Rewrote " << savedMaterialCount << " materials";
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(RewriteMaterials, "")

static void runVolumeConsoleCommand(const Vector<UnicodeString>& parameters)
{
    sounds().setMasterVolume(parameters[0].asFloat());
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(Volume, "<volume>")

static void runMuteConsoleCommand(const Vector<UnicodeString>& parameters)
{
    sounds().setMuted(parameters.empty() ? !sounds().isMuted() : parameters[0].asBoolean());
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(Mute, "[<true|false>]")

static void runListSoundShadersConsoleCommand(const Vector<UnicodeString>& parameters)
{
    console().printInColumns(U(soundShaders().getSoundShaderNames()), true);
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(ListSoundShaders, "")

static void runSetSoundShaderConsoleCommand(const Vector<UnicodeString>& parameters)
{
    auto soundShader = soundShaders().getSoundShader(A(parameters[0]));
    if (!soundShader)
    {
        LOG_CONSOLE << "Unknown sound shader: " << parameters[0];
        return;
    }

    if (parameters[1].asLower() == "volume")
        soundShader->setVolume(parameters[2].asFloat());
    else if (parameters[1].asLower() == "pitch")
        soundShader->setPitch(parameters[2].asFloat());
    else if (parameters[1].asLower() == "looping")
        soundShader->setLooping(parameters[2].asBoolean());
    else if (parameters[1].asLower() == "radius")
        soundShader->setRadius(parameters[2].asFloat());
    else
        LOG_CONSOLE << "Unknown sound shader parameter: " << parameters[1];
}
static void autocompleteSetSoundShaderConsoleCommand(unsigned int parameterIndex, Vector<UnicodeString>& completions)
{
    if (parameterIndex == 0)
        completions = U(soundShaders().getSoundShaderNames());
    else if (parameterIndex == 1)
        completions = {"Volume", "Pitch", "Looping", "Radius"};
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND_WITH_AUTOCOMPLETE(SetSoundShader, "<name> <property> <value>")

static void runSetEntityAlphaConsoleCommand(const Vector<UnicodeString>& parameters)
{
    auto entity = Scene::getSceneEntity(A(parameters[0]), A(parameters[1]));
    if (!entity)
    {
        LOG_CONSOLE << "Unknown scene or entity";
        return;
    }

    entity->setAlpha(parameters[2].asFloat());
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(SetEntityAlpha, "<scene> <entity> <alpha>")

static void runGraphicsDataBuffersConsoleCommand(const Vector<UnicodeString>& parameters)
{
    LOG_CONSOLE << dataBuffers().getMemoryStatistics();
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(GraphicsDataBuffers, "")

static void runOculusRiftModeConsoleCommand(const Vector<UnicodeString>& parameters)
{
    auto mode = Scene::OculusRiftMode();
    if (parameters[0].asLower() == "disabled")
        mode = Scene::OculusRiftDisabled;
    else if (parameters[0].asLower() == "enabled")
        mode = Scene::OculusRiftAndDefaultOutput;
    else if (parameters[0].asLower() == "exclusive")
        mode = Scene::OculusRiftExclusive;
    else
    {
        LOG_CONSOLE << "Invalid oculus rift mode: " << parameters[0];
        return;
    }

    auto sceneName = parameters.size() > 1 ? parameters[1].toASCII() : String::Empty;

    for (auto scene : Scene::getAllScenes())
    {
        if (!sceneName.length() || scene->getName() == sceneName)
            scene->setOculusRiftMode(mode);
    }
}
static void autocompleteOculusRiftModeConsoleCommand(unsigned int parameterIndex, Vector<UnicodeString>& completions)
{
    if (parameterIndex == 0)
        completions = {"Disabled", "Enabled", "Exclusive"};
    else if (parameterIndex == 1)
        completions = Scene::getAllScenes().map<UnicodeString>([](Scene* scene) { return scene->getName(); });
}
CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND_WITH_AUTOCOMPLETE(OculusRiftMode, "<mode> [<scene name>]")

}
