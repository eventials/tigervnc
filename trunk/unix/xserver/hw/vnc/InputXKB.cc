/* Copyright (C) 2009 TightVNC Team
 * Copyright (C) 2009 Red Hat, Inc.
 * Copyright 2013 Pierre Ossman for Cendio AB
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "Input.h"
#include "xorg-version.h"

#if XORG >= 17

extern "C" {
#define public c_public
#define class c_class
#include "xkbsrv.h"
#include "xkbstr.h"
#include "eventstr.h"
#include "scrnintstr.h"
#include "mi.h"
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#undef public
#undef class
}

#if XORG < 19
static int vncXkbScreenPrivateKeyIndex;
static DevPrivateKey vncXkbScreenPrivateKey = &vncXkbScreenPrivateKeyIndex;
#else
static DevPrivateKeyRec vncXkbPrivateKeyRec;
#define vncXkbScreenPrivateKey (&vncXkbPrivateKeyRec)
#endif

#define vncXkbScreenPrivate(pScreen) \
	(*(InputDevice**) dixLookupPrivate(&(pScreen)->devPrivates, \
	                                   vncXkbScreenPrivateKey))

#ifndef KEYBOARD_OR_FLOAT
#define KEYBOARD_OR_FLOAT MASTER_KEYBOARD
#endif

/* Stolen from libX11 */
static Bool
XkbTranslateKeyCode(register XkbDescPtr xkb, KeyCode key,
                    register unsigned int mods, unsigned int *mods_rtrn,
                    KeySym *keysym_rtrn)
{
	XkbKeyTypeRec *type;
	int col,nKeyGroups;
	unsigned preserve,effectiveGroup;
	KeySym *syms;

	if (mods_rtrn!=NULL)
		*mods_rtrn = 0;

	nKeyGroups= XkbKeyNumGroups(xkb,key);
	if ((!XkbKeycodeInRange(xkb,key))||(nKeyGroups==0)) {
		if (keysym_rtrn!=NULL)
			*keysym_rtrn = NoSymbol;
		return False;
	}

	syms = XkbKeySymsPtr(xkb,key);

	/* find the offset of the effective group */
	col = 0;
	effectiveGroup= XkbGroupForCoreState(mods);
	if ( effectiveGroup>=nKeyGroups ) {
		unsigned groupInfo= XkbKeyGroupInfo(xkb,key);
		switch (XkbOutOfRangeGroupAction(groupInfo)) {
		default:
			effectiveGroup %= nKeyGroups;
			break;
		case XkbClampIntoRange:
			effectiveGroup = nKeyGroups-1;
			break;
		case XkbRedirectIntoRange:
			effectiveGroup = XkbOutOfRangeGroupNumber(groupInfo);
			if (effectiveGroup>=nKeyGroups)
				effectiveGroup= 0;
			break;
		}
	}
	col= effectiveGroup*XkbKeyGroupsWidth(xkb,key);
	type = XkbKeyKeyType(xkb,key,effectiveGroup);

	preserve= 0;
	if (type->map) { /* find the column (shift level) within the group */
		register int i;
		register XkbKTMapEntryPtr entry;
		for (i=0,entry=type->map;i<type->map_count;i++,entry++) {
			if ((entry->active)&&((mods&type->mods.mask)==entry->mods.mask)) {
				col+= entry->level;
				if (type->preserve)
					preserve= type->preserve[i].mask;
				break;
			}
		}
	}

	if (keysym_rtrn!=NULL)
		*keysym_rtrn= syms[col];
	if (mods_rtrn)
		*mods_rtrn= type->mods.mask&(~preserve);

	return (syms[col]!=NoSymbol);
}

static XkbAction *XkbKeyActionPtr(XkbDescPtr xkb, KeyCode key, unsigned int mods)
{
	XkbKeyTypeRec *type;
	int col,nKeyGroups;
	unsigned effectiveGroup;
	XkbAction *acts;

	if (!XkbKeyHasActions(xkb, key))
		return NULL;

	nKeyGroups= XkbKeyNumGroups(xkb,key);
	if ((!XkbKeycodeInRange(xkb,key))||(nKeyGroups==0))
		return NULL;

	acts = XkbKeyActionsPtr(xkb,key);

	/* find the offset of the effective group */
	col = 0;
	effectiveGroup= XkbGroupForCoreState(mods);
	if ( effectiveGroup>=nKeyGroups ) {
		unsigned groupInfo= XkbKeyGroupInfo(xkb,key);
		switch (XkbOutOfRangeGroupAction(groupInfo)) {
		default:
			effectiveGroup %= nKeyGroups;
			break;
		case XkbClampIntoRange:
			effectiveGroup = nKeyGroups-1;
			break;
		case XkbRedirectIntoRange:
			effectiveGroup = XkbOutOfRangeGroupNumber(groupInfo);
			if (effectiveGroup>=nKeyGroups)
				effectiveGroup= 0;
			break;
		}
	}
	col= effectiveGroup*XkbKeyGroupsWidth(xkb,key);
	type = XkbKeyKeyType(xkb,key,effectiveGroup);

	if (type->map) { /* find the column (shift level) within the group */
		register int i;
		register XkbKTMapEntryPtr entry;
		for (i=0,entry=type->map;i<type->map_count;i++,entry++) {
			if ((entry->active)&&((mods&type->mods.mask)==entry->mods.mask)) {
				col+= entry->level;
				break;
			}
		}
	}

	return &acts[col];
}

void InputDevice::PrepareInputDevices(void)
{
#if XORG < 19
	if (!dixRequestPrivate(vncXkbScreenPrivateKey, sizeof(InputDevice*)))
		FatalError("Failed to register TigerVNC XKB screen key\n");
#else
	if (!dixRegisterPrivateKey(vncXkbScreenPrivateKey, PRIVATE_SCREEN,
	                           sizeof(InputDevice*)))
		FatalError("Failed to register TigerVNC XKB screen key\n");
#endif

	for (int scr = 0; scr < screenInfo.numScreens; scr++)
		vncXkbScreenPrivate(screenInfo.screens[scr]) = this;

	/*
	 * Not ideal since these callbacks do not stack, but it's the only
	 * decent way we can reliably catch events for both the slave and
	 * master device.
	 */
	mieqSetHandler(ET_KeyPress, vncXkbProcessDeviceEvent);
	mieqSetHandler(ET_KeyRelease, vncXkbProcessDeviceEvent);
}

unsigned InputDevice::getKeyboardState(void)
{
	DeviceIntPtr master;

	master = GetMaster(keyboardDev, KEYBOARD_OR_FLOAT);
	return XkbStateFieldFromRec(&master->key->xkbInfo->state);
}

unsigned InputDevice::getLevelThreeMask(void)
{
	KeyCode keycode;
	XkbDescPtr xkb;
	XkbAction *act;

	keycode = keysymToKeycode(XK_ISO_Level3_Shift, 0, NULL);
	if (keycode == 0) {
		keycode = keysymToKeycode(XK_Mode_switch, 0, NULL);
		if (keycode == 0)
			return 0;
	}

	xkb = GetMaster(keyboardDev, KEYBOARD_OR_FLOAT)->key->xkbInfo->desc;

	act = XkbKeyActionPtr(xkb, keycode, 0);
	if (act == NULL)
		return 0;
	if (act->type != XkbSA_SetMods)
		return 0;

	if (act->mods.flags & XkbSA_UseModMapMods)
		return xkb->map->modmap[keycode];
	else
		return act->mods.mask;
}

KeyCode InputDevice::pressShift(void)
{
	unsigned state;

	XkbDescPtr xkb;
	unsigned int key;

	state = getKeyboardState();
	if (state & ShiftMask)
		return 0;

	xkb = GetMaster(keyboardDev, KEYBOARD_OR_FLOAT)->key->xkbInfo->desc;
	for (key = xkb->min_key_code; key <= xkb->max_key_code; key++) {
		XkbAction *act;
		unsigned char mask;

		act = XkbKeyActionPtr(xkb, key, state);
		if (act == NULL)
			continue;

		if (act->type != XkbSA_SetMods)
			continue;

		if (act->mods.flags & XkbSA_UseModMapMods)
			mask = xkb->map->modmap[key];
		else
			mask = act->mods.mask;

		if ((mask & ShiftMask) == ShiftMask)
			return key;
	}

	return 0;
}

std::list<KeyCode> InputDevice::releaseShift(void)
{
	unsigned state;
	std::list<KeyCode> keys;

	DeviceIntPtr master;
	XkbDescPtr xkb;
	unsigned int key;

	state = getKeyboardState();
	if (!(state & ShiftMask))
		return keys;

	master = GetMaster(keyboardDev, KEYBOARD_OR_FLOAT);
	xkb = master->key->xkbInfo->desc;
	for (key = xkb->min_key_code; key <= xkb->max_key_code; key++) {
		XkbAction *act;
		unsigned char mask;

		if (!key_is_down(master, key, KEY_PROCESSED))
			continue;

		act = XkbKeyActionPtr(xkb, key, state);
		if (act == NULL)
			continue;

		if (act->type != XkbSA_SetMods)
			continue;

		if (act->mods.flags & XkbSA_UseModMapMods)
			mask = xkb->map->modmap[key];
		else
			mask = act->mods.mask;

		if (!(mask & ShiftMask))
			continue;

		keys.push_back(key);
	}

	return keys;
}

KeyCode InputDevice::pressLevelThree(void)
{
	unsigned state, mask;

	KeyCode keycode;
	XkbDescPtr xkb;
	XkbAction *act;

	mask = getLevelThreeMask();
	if (mask == 0)
		return 0;

	state = getKeyboardState();
	if (state & mask)
		return 0;

	keycode = keysymToKeycode(XK_ISO_Level3_Shift, state, NULL);
	if (keycode == 0) {
		keycode = keysymToKeycode(XK_Mode_switch, state, NULL);
		if (keycode == 0)
			return 0;
	}

	xkb = GetMaster(keyboardDev, KEYBOARD_OR_FLOAT)->key->xkbInfo->desc;

	act = XkbKeyActionPtr(xkb, keycode, 0);
	if (act == NULL)
		return 0;
	if (act->type != XkbSA_SetMods)
		return 0;

	return keycode;
}

std::list<KeyCode> InputDevice::releaseLevelThree(void)
{
	unsigned state, mask;
	std::list<KeyCode> keys;

	DeviceIntPtr master;
	XkbDescPtr xkb;
	unsigned int key;

	mask = getLevelThreeMask();
	if (mask == 0)
		return keys;

	state = getKeyboardState();
	if (!(state & mask))
		return keys;

	master = GetMaster(keyboardDev, KEYBOARD_OR_FLOAT);
	xkb = master->key->xkbInfo->desc;
	for (key = xkb->min_key_code; key <= xkb->max_key_code; key++) {
		XkbAction *act;
		unsigned char key_mask;

		if (!key_is_down(master, key, KEY_PROCESSED))
			continue;

		act = XkbKeyActionPtr(xkb, key, state);
		if (act == NULL)
			continue;

		if (act->type != XkbSA_SetMods)
			continue;

		if (act->mods.flags & XkbSA_UseModMapMods)
			key_mask = xkb->map->modmap[key];
		else
			key_mask = act->mods.mask;

		if (!(key_mask & mask))
			continue;

		keys.push_back(key);
	}

	return keys;
}

KeyCode InputDevice::keysymToKeycode(KeySym keysym, unsigned state,
                                     unsigned *new_state)
{
	XkbDescPtr xkb;
	unsigned int key;
	KeySym ks;
	unsigned level_three_mask;

	if (new_state != NULL)
		*new_state = state;

	xkb = GetMaster(keyboardDev, KEYBOARD_OR_FLOAT)->key->xkbInfo->desc;
	for (key = xkb->min_key_code; key <= xkb->max_key_code; key++) {
		unsigned int state_out;
		KeySym dummy;

		XkbTranslateKeyCode(xkb, key, state, &state_out, &ks);
		if (ks == NoSymbol)
			continue;

		/*
		 * Despite every known piece of documentation on
		 * XkbTranslateKeyCode() stating that mods_rtrn returns
		 * the unconsumed modifiers, in reality it always
		 * returns the _potentially consumed_ modifiers.
		 */
		state_out = state & ~state_out;
		if (state_out & LockMask)
			XkbConvertCase(ks, &dummy, &ks);

		if (ks == keysym)
			return key;
	}

	if (new_state == NULL)
		return 0;

	*new_state = (state & ~ShiftMask) |
	             ((state & ShiftMask) ? 0 : ShiftMask);
	key = keysymToKeycode(keysym, *new_state, NULL);
	if (key != 0)
		return key;

	level_three_mask = getLevelThreeMask();
	if (level_three_mask == 0)
		return 0;

	*new_state = (state & ~level_three_mask) | 
	             ((state & level_three_mask) ? 0 : level_three_mask);
	key = keysymToKeycode(keysym, *new_state, NULL);
	if (key != 0)
		return key;

	*new_state = (state & ~(ShiftMask | level_three_mask)) | 
	             ((state & ShiftMask) ? 0 : ShiftMask) |
	             ((state & level_three_mask) ? 0 : level_three_mask);
	key = keysymToKeycode(keysym, *new_state, NULL);
	if (key != 0)
		return key;

	return 0;
}

bool InputDevice::isLockModifier(KeyCode keycode, unsigned state)
{
	XkbDescPtr xkb;
	XkbAction *act;

	xkb = GetMaster(keyboardDev, KEYBOARD_OR_FLOAT)->key->xkbInfo->desc;

	act = XkbKeyActionPtr(xkb, keycode, state);
	if (act == NULL)
		return false;

	if (act->type != XkbSA_LockMods)
		return false;

	return true;
}

KeyCode InputDevice::addKeysym(KeySym keysym, unsigned state)
{
	DeviceIntPtr master;
	XkbDescPtr xkb;
	unsigned int key;

	XkbEventCauseRec cause;
	XkbChangesRec changes;

	int types[1];
	KeySym *syms;

	master = GetMaster(keyboardDev, KEYBOARD_OR_FLOAT);
	xkb = master->key->xkbInfo->desc;
	for (key = xkb->max_key_code; key >= xkb->min_key_code; key--) {
		if (XkbKeyNumGroups(xkb, key) == 0)
			break;
	}

	if (key < xkb->min_key_code)
		return 0;

	memset(&changes, 0, sizeof(changes));
	memset(&cause, 0, sizeof(cause));

	XkbSetCauseCoreReq(&cause, X_ChangeKeyboardMapping, NULL);

	/* FIXME: Verify that ONE_LEVEL isn't screwed up */

	types[XkbGroup1Index] = XkbOneLevelIndex;
	XkbChangeTypesOfKey(xkb, key, 1, XkbGroup1Mask, types, &changes.map);

	syms = XkbKeySymsPtr(xkb,key);
	syms[0] = keysym;

	changes.map.changed |= XkbKeySymsMask;
	changes.map.first_key_sym = key;
	changes.map.num_key_syms = 1;

	XkbSendNotification(master, &changes, &cause);

	return key;
}

void InputDevice::vncXkbProcessDeviceEvent(int screenNum,
                                           InternalEvent *event,
                                           DeviceIntPtr dev)
{
	InputDevice *self = vncXkbScreenPrivate(screenInfo.screens[screenNum]);
	unsigned int backupctrls;

	if (event->device_event.sourceid == self->keyboardDev->id) {
		XkbControlsPtr ctrls;

		/*
		 * We need to bypass AccessX since it is timing sensitive and
		 * the network can cause fake event delays.
		 */
		ctrls = dev->key->xkbInfo->desc->ctrls;
		backupctrls = ctrls->enabled_ctrls;
		ctrls->enabled_ctrls &= ~XkbAllFilteredEventsMask;

		/*
		 * This flag needs to be set for key repeats to be properly
		 * respected.
		 */
		if ((event->device_event.type == ET_KeyPress) &&
		    key_is_down(dev, event->device_event.detail.key, KEY_PROCESSED))
			event->device_event.key_repeat = TRUE;
	}

	dev->c_public.processInputProc(event, dev);

	if (event->device_event.sourceid == self->keyboardDev->id) {
		XkbControlsPtr ctrls;

		ctrls = dev->key->xkbInfo->desc->ctrls;
		ctrls->enabled_ctrls = backupctrls;
	}
}

#endif /* XORG >= 117 */
