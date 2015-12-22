/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Render/Effect.h"

namespace Carbon
{

/**
 * Handles the loading and storage of the registered effect definitions.
 */
class CARBON_API EffectManager : private Noncopyable
{
public:

    /**
     * Parses all the .effect files in the effects directory and loads in all defined effects. By default
     * Effect::updateActiveShader() will be called on each loaded effect, but this can be skipped if no rendering is going to be
     * done (this is used by exporters).
     */
    void loadEffects(bool updateActiveShaders = true);

    /**
     * Returns the effects vector.
     */
    const Vector<Effect*>& getEffects() const { return effects_; }

    /**
     * Returns a vector containing the names of all available effects.
     */
    Vector<String> getEffectNames() const;

    /**
     * Returns the index of the effect with the given name, or null if it is not found.
     */
    Effect* getEffect(const String& name);

    /**
     * Returns the active shader for the given effect by calling its Effect::getActiveShader() method. This is a helper method
     * that returns null if the effect doesn't exist.
     */
    Shader* getEffectActiveShader(const String& name);

    /**
     * Returns a vector containing the shaders active on each loaded effect. Effects with no active shader are skipped.
     */
    Vector<Shader*> getAllActiveShaders();

    /**
     * This method is used by the renderer to notify the font manager of a RecreateWindowEvent that it needs to process.
     */
    void onRecreateWindowEvent(const RecreateWindowEvent& rwe);

private:

    EffectManager() {}
    ~EffectManager() { clear(); }
    friend class Globals;

    void clear();

    Vector<Effect*> effects_;
};

}
