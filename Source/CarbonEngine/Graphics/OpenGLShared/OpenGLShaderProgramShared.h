/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Graphics/ShaderConstant.h"
#include "CarbonEngine/Graphics/ShaderProgram.h"

namespace Carbon
{

/**
 * This is a partial ShaderProgram implementation for OpenGL and GLSL that contains code shared by all of the OpenGL
 * backends.
 */
class OpenGLShaderProgramShared : public ShaderProgram
{
public:

    OpenGLShaderProgramShared(ShaderLanguage language) : ShaderProgram(language) {}

    GLuint getProgram() const { return program_; }

    void clear() override
    {
        if (program_)
        {
            deleteProgram();
            program_ = 0;
        }

        shaderSource_.clear();

        ShaderProgram::clear();
    }

    bool addSource(const String& source, const String& filename) override
    {
        auto glShaderType = getOpenGLShaderType(filename);
        if (!glShaderType)
            return false;

        shaderSource_[glShaderType].filenames.append(filename);
        shaderSource_[glShaderType].source << source;

        return true;
    }

    bool link() override
    {
        try
        {
            program_ = createProgram();

            // Compile all the final source code for this shader
            for (const auto& shaderSource : shaderSource_)
            {
                auto filenames = String(shaderSource.second.filenames, " + ");

                auto source =
                    getSourcePrefix(shaderSource.first) + getPreprocessorDefines() + shaderSource.second.source;

                // Run GLSL processing
                prepareGLSL(filenames, source);

                // Create a new OpenGL shader
                auto glShader = createShader(shaderSource.first);
                if (!glShader)
                    throw Exception("Failed creating OpenGL shader object");

                // Compile the shader source
                auto isCompiled = compileShader(glShader, source);
                auto compilerOutput = getCompilerOutput(glShader).getLines();
                if (compilerOutput == Vector<String>(1, "No errors."))
                    compilerOutput.clear();

                // Full shader source code is logged if there was an error or some kind of output from the compiler
                if (!isCompiled || compilerOutput.size())
                    logShaderCode(filenames, source);

                // Write compiler output to the logfile
                if (compilerOutput.size())
                {
                    compilerOutput.prepend("Compiler output for " + filenames + ":");
                    Logfile::get().writeLines(compilerOutput, isCompiled ? Logfile::Info : Logfile::Error);
                }

                // Attach shader to the program if it compiled
                if (isCompiled)
                    attachShader(glShader);

                deleteShader(glShader);

                if (!isCompiled)
                    throw Exception("Shader compile failed");
            }

            // Link the program
            auto isLinked = linkProgram();

            // Check linker output
            auto linkerOutput = getLinkerOutput().getLines();
            if (linkerOutput == Vector<String>{{"No errors."}})
                linkerOutput.clear();

            // Log the linker output
            if (linkerOutput.size())
            {
                linkerOutput.prepend("Linker output for " + String(getSourceFilenames(), " + ") + ":");
                Logfile::get().writeLines(linkerOutput, isLinked ? Logfile::Info : Logfile::Error);
            }

            if (!isLinked)
                throw Exception("Link failed");

            return true;
        }
        catch (const Exception& e)
        {
            LOG_ERROR << e;

            return false;
        }
    }

protected:

    virtual GLenum getOpenGLShaderType(const String& filename) const
    {
        LOG_ERROR << filename << " - Unknown shader type";
        return 0;
    }

private:

    GLuint program_ = 0;

    // Shader source code bucketed by shader type
    struct ShaderTypeSource
    {
        Vector<String> filenames;
        String source;
    };
    std::unordered_map<GLenum, ShaderTypeSource> shaderSource_;

    Vector<String> getSourceFilenames() const
    {
        auto filenames = Vector<String>();

        // Create list of all the source files and cut off directory names
        for (auto& shaderTypeSource : shaderSource_)
        {
            for (auto& filename : shaderTypeSource.second.filenames)
                filenames.append(A(FileSystem::getBaseName(filename)));
        }

        return filenames;
    }

    void prepareGLSL(const String& filename, String& glsl)
    {
        auto lines = glsl.getLines();

        preprocessShaderCode(filename, lines);

        // Remove duplicate declarations, this works for GLSL 1.10 and GLSL 1.50
        removeDuplicateLinesWithPrefix(lines, "attribute ");
        removeDuplicateLinesWithPrefix(lines, "uniform ");
        removeDuplicateLinesWithPrefix(lines, "varying ");
        removeDuplicateLinesWithPrefix(lines, "in ");
        removeDuplicateLinesWithPrefix(lines, "out ");

        // Get rid of consecutive blank lines that may have been left in by the above processing
        for (auto i = 1U; i < lines.size(); i++)
        {
            if (lines[i - 1].length() == 0 && lines[i].length() == 0)
                lines.erase(i--);
        }
        if (lines.size() && lines.back().length() == 0)
            lines.popBack();

        glsl = String(lines, String::Newline);
    }

    void removeDuplicateLinesWithPrefix(Vector<String>& lines, const String& prefix)
    {
        auto found = Vector<String>();

        for (auto i = 0U; i < lines.size(); i++)
        {
            auto trimmed = lines[i].trimmed();
            if (trimmed.startsWith(prefix))
            {
                if (found.has(trimmed))
                    lines.erase(i--);
                else
                    found.append(trimmed);
            }
        }
    }

    // Subclasses must implement the following methods
    virtual GLuint createProgram() = 0;
    virtual void deleteProgram() = 0;
    virtual String getSourcePrefix(GLenum glShaderType) = 0;
    virtual bool linkProgram() = 0;
    virtual String getLinkerOutput() = 0;
    virtual GLuint createShader(GLenum glShaderType) = 0;
    virtual void deleteShader(GLuint glShader) = 0;
    virtual bool compileShader(GLuint glShader, const String& source) = 0;
    virtual String getCompilerOutput(GLuint glShader) = 0;
    virtual void attachShader(GLuint glShader) = 0;
};

}
