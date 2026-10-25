// SPDX-License-Identifier: GPL-2.0-or-later

package org.dolphinemu.dolphinemu.activities

import android.app.Activity
import android.os.Bundle

/**
 * Lets Android offer to always hand the GameCube controller adapter over to Dolphin when it is
 * plugged in. The running game picks the adapter up by itself.
 */
class GCAdapterActivity : Activity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        finish()
    }
}
