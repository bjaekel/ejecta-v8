package ag.boersego.bgjs.modules

import ag.boersego.bgjs.*
import android.content.Context
import android.content.SharedPreferences

/**
 * Created by Kevin Read <me@kevin-read.com> on 23.11.17 for myrmecophaga-2.0.
 * Copyright (c) 2017 BörseGo AG. All rights reserved.
 */

class BGJSModuleLocalStorage private constructor(applicationContext: Context) : JNIV8Module("localStorage") {

    private var preferences: SharedPreferences

    init {
        preferences = applicationContext.getSharedPreferences(TAG, 0)
    }

    // In order to provide a key(n) method similar to Web Storage, we need to remember the insertion order

    override fun Require(engine: V8Engine, module: JNIV8GenericObject?) {
        val exports = JNIV8GenericObject.Create(engine)

        //TODO: Can we do this as Attribute?
        exports.setV8Field("length", JNIV8Function.Create(engine) { receiver, arguments ->
            if (!arguments.isEmpty()) {
                throw IllegalArgumentException("clear needs no arguments")
            }
            preferences.all.size - 1
        })
        exports.setV8Field("clear", JNIV8Function.Create(engine) { receiver, arguments ->
            if (!arguments.isEmpty()) {
                throw IllegalArgumentException("clear needs no arguments")
            }
            preferences.edit().clear().apply()
            JNIV8Undefined.GetInstance()
        })

        exports.setV8Field("getItem", JNIV8Function.Create(engine) { receiver, arguments ->
            if (arguments.isEmpty() || arguments[0] !is String) {
                throw IllegalArgumentException("getItem needs one parameter of type String")
            }
            val key = arguments[0] as String

            preferences.getString(key, null)
        })

        // From HTML Standard:
        // The key(n) method must return the name of the nth key in the list.
        // The order of keys is user-agent defined, but must be consistent within an object so long as the number of keys doesn't change.
        // (Thus, adding or removing a key may change the order of the keys, but merely changing the value of an existing key must not.)
        // If n is greater than or equal to the number of key/value pairs in the object, then this method must return null.

        exports.setV8Field("key", JNIV8Function.Create(engine) { receiver, arguments ->
            if (arguments.isEmpty() || arguments[0] !is Double) {
                throw IllegalArgumentException("getItem needs one parameter of type Number")
            }
            val index = (arguments[0] as Double).toInt()
            val orderedKeys = preferences.getString(INSERT_ORDER, null)?.split(",")
            if (orderedKeys != null && index < orderedKeys.size) {
                orderedKeys[index]
            } else {
                null
            }
        })

        exports.setV8Field("removeItem", JNIV8Function.Create(engine) { receiver, arguments ->
            if (arguments.isEmpty() || arguments[0] !is String) {
                throw IllegalArgumentException("removeItem needs one parameter of type String")
            }
            val key = arguments[0] as String
            preferences.edit().remove(key).apply()

            var insertOrder = preferences.getString(INSERT_ORDER, "")
            insertOrder = insertOrder!!.replace("$key,", "")
            preferences.edit().putString(INSERT_ORDER, insertOrder).apply()

            JNIV8Undefined.GetInstance()
        })

        exports.setV8Field("setItem", JNIV8Function.Create(engine) { _, arguments ->
            if (arguments.size < 2 || arguments[0] !is String || arguments[1] !is String) {
                throw IllegalArgumentException("setItem needs two parameters of type String")
            }
            val key = arguments[0] as String
            val value = arguments[1] as String
            preferences.edit().putString(key, value).apply()

            val insertOrder = preferences.getString(INSERT_ORDER, "")
            if (!insertOrder!!.contains(key)) preferences.edit().putString(INSERT_ORDER, "$insertOrder$key,").apply()

            JNIV8Undefined.GetInstance()
        })

        module?.setV8Field("exports", exports)
    }

    fun registerPreferencesListener(listener: SharedPreferences.OnSharedPreferenceChangeListener) {
        preferences.registerOnSharedPreferenceChangeListener(listener)
    }

    fun getItem(key: String) : String {
        return preferences.getString(key, "") ?: ""
    }

    companion object {
        private val TAG = BGJSModuleLocalStorage::class.java.simpleName
        private const val INSERT_ORDER = "insert_order"
        @Volatile private var instance: BGJSModuleLocalStorage? = null

        const val PREF_PORTAL = "auth:portal"

        @JvmStatic
        fun getInstance(ctx : Context) : BGJSModuleLocalStorage {
            val i = instance
            if(i != null) {
                return i
            }

            return synchronized(this) {
                val i2 = instance
                if(i2 != null) {
                    i2
                } else {
                    val created = BGJSModuleLocalStorage(ctx)
                    instance = created
                    created
                }
            }
        }
    }
}