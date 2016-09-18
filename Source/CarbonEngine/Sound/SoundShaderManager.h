/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Manages all loaded sound shaders with reference counting, the sound shaders are applied to sound sources in the sound
 * layer. See the SoundShader and SoundInterface classes for details.
 */
class CARBON_API SoundShaderManager : private Noncopyable
{
public:

    /**
     * Unloads all loaded sound shaders.
     */
    void clear();

    /**
     * Takes a reference to a sound shader, the sound shader will be loaded if it isn't currently being used or the
     * reference count of the already loaded sound shader will be increased. The reference to the sound shader that is
     * taken by this method must always be released using SoundShaderManager::releaseSoundShader(). Returns a pointer to
     * the sound shader object.
     */
    const SoundShader* setupSoundShader(const String& name);

    /**
     * Releases a sound shader reference that was returned by SoundShaderManager::setupSoundShader(). Releasing a null
     * pointer is a no-op.
     */
    void releaseSoundShader(const SoundShader* soundShader);

    /**
     * Returns a pointer to a loaded sound shader, or null if there is no loaded sound shader with the given name.
     */
    SoundShader* getSoundShader(const String& name);

    /**
     * Returns the names of all the sound shaders that are currently loaded.
     */
    Vector<String> getSoundShaderNames() const;

private:

    SoundShaderManager() {}
    ~SoundShaderManager() { clear(); }
    friend class Globals;

    Vector<SoundShader*> soundShaders_;
};

}
