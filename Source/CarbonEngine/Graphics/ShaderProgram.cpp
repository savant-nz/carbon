/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Graphics/ShaderConstant.h"
#include "CarbonEngine/Graphics/ShaderProgram.h"

namespace Carbon
{

void ShaderProgram::clear()
{
    language_ = NoShaderLanguage;
    preprocessorDefines_.clear();

    for (auto& constant : constants_)
        delete constant.constant;
    constants_.clear();
}

ShaderConstant* ShaderProgram::getConstant(const String& name, const String& parameterName)
{
    for (auto& constant : constants_)
    {
        if (constant.name == name)
            return constant.constant;
    }

    auto constant = getConstantUncached(name, parameterName);
    if (!constant)
        LOG_WARNING << "Named constant doesn't exist in this shader program: " << name;

    constants_.emplace(name, constant);

    return constant;
}

struct PreprocessorConditionalBlock
{
    bool active = false;
    unsigned int startLine = 0U;
    bool keepContents = false;

    PreprocessorConditionalBlock() {}
    PreprocessorConditionalBlock(bool active_, unsigned int startLine_, bool keepContents_)
        : active(active_), startLine(startLine_), keepContents(keepContents_)
    {
    }
};

bool ShaderProgram::preprocessShaderCode(const String& filename, Vector<String>& lines)
{
    // Figure out the base #include path to use
    auto includePath = filename.has('/') ? filename.substr(0, filename.findLastOf("/") + 1) : "";

    auto defines = std::unordered_map<String, String>();
    auto conditionalBlocks = Vector<PreprocessorConditionalBlock>();

    for (auto i = 0U; i < lines.size(); i++)
    {
        lines[i].trimRight();

        auto line = lines[i].trimmedLeft();

        if (!line.startsWith("#"))
            continue;

        if (line.startsWith("#include"))
        {
            const auto& tokens = lines[i].getTokens();
            if (tokens.size() == 2)
            {
                lines[i].clear();

                auto contents = String();
                auto includedFile = FileReader();
                if (!fileSystem().readTextFile(includePath + tokens[1].withoutSuffix("\"").withoutPrefix("\""), contents))
                    LOG_WARNING << "Failed including file '" << tokens[1] << "' in shader '" << filename << "'";
                else
                    lines.insert(i + 1, contents.getLines());
            }
        }
        else if (line.startsWith("#define"))
        {
            if (conditionalBlocks.size() && conditionalBlocks.back().active && !conditionalBlocks.back().keepContents)
                continue;

            const auto& tokens = lines[i].getTokens();
            if (tokens.size() >= 2)
                defines[tokens[1]].clear();
        }
        else if (line.startsWith("#undef"))
        {
            if (conditionalBlocks.size() && conditionalBlocks.back().active && !conditionalBlocks.back().keepContents)
                continue;

            const auto& tokens = lines[i].getTokens();
            if (tokens.size() >= 2)
                defines.erase(tokens[1]);
        }
        else if (line.startsWith("#ifdef") || lines[i].startsWith("#ifndef"))
        {
            const auto& tokens = lines[i].getTokens();
            if (tokens.size() != 2)
            {
                LOG_WARNING << "Encountered unsupported preprocessor command while parsing " << filename << ": " << lines[i];
                continue;
            }

            // Start conditional block
            if (lines[i].startsWith("#ifdef"))
                conditionalBlocks.emplace(true, i, defines.find(tokens[1]) != defines.end());
            else
                conditionalBlocks.emplace(true, i, defines.find(tokens[1]) == defines.end());

            lines[i].clear();
        }
        else if (line.startsWith("#if"))
        {
            // #if's are ignored, so start a fake conditional block
            conditionalBlocks.append(PreprocessorConditionalBlock());
        }
        else if (line.startsWith("#else"))
        {
            if (conditionalBlocks.size())
            {
                if (conditionalBlocks.back().active)
                {
                    lines[i].clear();

                    // Get rid of the contents up to this #else if it's not wanted
                    if (!conditionalBlocks.back().keepContents)
                    {
                        for (auto j = conditionalBlocks.back().startLine; j <= i; j++)
                            lines[j].clear();
                    }

                    // Flip the conditional block so it's ready for the #endif
                    conditionalBlocks.back().startLine = i;
                    conditionalBlocks.back().keepContents = !conditionalBlocks.back().keepContents;
                }
            }
            else
                LOG_WARNING << "Encountered an unmatched #else command while parsing " << filename;
        }
        else if (line.startsWith("#endif"))
        {
            if (conditionalBlocks.size())
            {
                if (conditionalBlocks.back().active)
                {
                    lines[i].clear();

                    // Get rid of the contents up to this #endif if it's not wanted
                    if (!conditionalBlocks.back().keepContents)
                    {
                        for (auto j = conditionalBlocks.back().startLine; j <= i; j++)
                            lines[j].clear();
                    }
                }

                conditionalBlocks.popBack();
            }
            else
                LOG_WARNING << "Encountered an unmatched #endif command while parsing " << filename;
        }
    }

    if (!conditionalBlocks.empty())
        LOG_WARNING << "Incomplete conditional preprocessor blocks while parsing " << filename;

    return true;
}

void ShaderProgram::logShaderCode(const String& filename, const String& shaderCode)
{
    Logfile::get().writeCollapsibleSection("Shader code for " + FileSystem::getBaseName(filename), U(shaderCode.getLines()),
                                           Logfile::Info, true);
}

}
