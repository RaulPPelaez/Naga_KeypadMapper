//EXPERIMENTAL
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>

#include <cstring>
#include <thread>

#ifndef FAKEKEYS_H_
#define FAKEKEYS_H_
#define N_MODIFIER_INDEXES (Mod5MapIndex + 1)

typedef enum
{
	FAKEKEYMOD_SHIFT = (1<<1),
	FAKEKEYMOD_CONTROL = (1<<2),
	FAKEKEYMOD_ALT = (1<<3),
	FAKEKEYMOD_META = (1<<4)

} FakeKeyModifier;

struct FakeKey
{
	Display *xdpy;
	int min_keycode, max_keycode;
	int n_keysyms_per_keycode;
	KeySym *keysyms;
	int held_keycode;
	int held_state_flags;
	KeyCode modifier_table[N_MODIFIER_INDEXES];
	int shift_mod_index, alt_mod_index, meta_mod_index;
};

static int utf8_to_ucs4 (const unsigned char *src_orig, unsigned int *dst, int len)
{
	const unsigned char *src = src_orig;
	unsigned char s;
	int extra;
	unsigned int result;

	if (len == 0)
		return 0;

	s = *src++;
	len--;

	if (!(s & 0x80))
	{
		result = s;
		extra = 0;
	}
	else if (!(s & 0x40))
	{
		return -1;
	}
	else if (!(s & 0x20))
	{
		result = s & 0x1f;
		extra = 1;
	}
	else if (!(s & 0x10))
	{
		result = s & 0xf;
		extra = 2;
	}
	else if (!(s & 0x08))
	{
		result = s & 0x07;
		extra = 3;
	}
	else if (!(s & 0x04))
	{
		result = s & 0x03;
		extra = 4;
	}
	else if ( !(s & 0x02))
	{
		result = s & 0x01;
		extra = 5;
	}
	else
	{
		return -1;
	}
	if (extra > len)
		return -1;

	while (extra--)
	{
		result <<= 6;
		s = *src++;

		if ((s & 0xc0) != 0x80)
			return -1;

		result |= s & 0x3f;
	}
	*dst = result;
	return src - src_orig;
}

FakeKey* fakekey_init(Display *xdpy)
{
	FakeKey *fk = NULL;
	int event, error, major, minor;
	XModifierKeymap *modifiers;
	int mod_index;
	int mod_key;
	KeyCode *kp;

	if (xdpy == NULL) return NULL;

	if (!XTestQueryExtension(xdpy, &event, &error, &major, &minor))
	{
		return NULL;
	}

	fk = (FakeKey*)malloc(sizeof(FakeKey));
	memset(fk,0,sizeof(FakeKey));

	fk->xdpy = xdpy;

	/* Find keycode limits */

	XDisplayKeycodes(fk->xdpy, &fk->min_keycode, &fk->max_keycode);

	/* Get the mapping */

	/* TODO: Below needs to be kept in sync with anything else
	 * that may change the keyboard mapping.
	 *
	 * case MappingNotify:
	 * XRefreshKeyboardMapping(&ev.xmapping);
	 *
	 */

	fk->keysyms = XGetKeyboardMapping(fk->xdpy,
	                                  fk->min_keycode,
	                                  fk->max_keycode - fk->min_keycode + 1,
	                                  &fk->n_keysyms_per_keycode);


	modifiers = XGetModifierMapping(fk->xdpy);

	kp = modifiers->modifiermap;

	for (mod_index = 0; mod_index < 8; mod_index++)
	{
		fk->modifier_table[mod_index] = 0;

		for (mod_key = 0; mod_key < modifiers->max_keypermod; mod_key++)
		{
			int keycode = kp[mod_index * modifiers->max_keypermod + mod_key];

			if (keycode != 0)
			{
				fk->modifier_table[mod_index] = keycode;
				break;
			}
		}
	}

	for (mod_index = Mod1MapIndex; mod_index <= Mod5MapIndex; mod_index++)
	{
		if (fk->modifier_table[mod_index])
		{
			KeySym ks = XKeycodeToKeysym(fk->xdpy,
			                             fk->modifier_table[mod_index], 0);

			/*
			 * Note: ControlMapIndex is already defined by xlib
			 * ShiftMapIndex
			 */

			switch (ks)
			{
			case XK_Meta_R:
			case XK_Meta_L:
				fk->meta_mod_index = mod_index;
				break;

			case XK_Alt_R:
			case XK_Alt_L:
				fk->alt_mod_index = mod_index;
				break;

			case XK_Shift_R:
			case XK_Shift_L:
				fk->shift_mod_index = mod_index;
				break;
			}
		}
	}

	if (modifiers)
		XFreeModifiermap(modifiers);

	return fk;
}

int fakekey_send_keyevent(FakeKey *fk, KeyCode keycode, Bool is_press, int flags)
{
	if (flags)
	{
		if (flags & FAKEKEYMOD_SHIFT)
			XTestFakeKeyEvent(fk->xdpy, fk->modifier_table[ShiftMapIndex],
			                  is_press, CurrentTime);

		if (flags & FAKEKEYMOD_CONTROL)
			XTestFakeKeyEvent(fk->xdpy, fk->modifier_table[ControlMapIndex],
			                  is_press, CurrentTime);

		if (flags & FAKEKEYMOD_ALT)
			XTestFakeKeyEvent(fk->xdpy, fk->modifier_table[fk->alt_mod_index],
			                  is_press, CurrentTime);

		XSync(fk->xdpy, False);
	}

	XTestFakeKeyEvent(fk->xdpy, keycode, is_press, CurrentTime);

	XSync(fk->xdpy, False);

	return 1;
}

int fakekey_press_keysym(FakeKey *fk, KeySym keysym, int flags)
{
	static int modifiedkey;
	KeyCode code = 0;

	if ((code = XKeysymToKeycode(fk->xdpy, keysym)) != 0)
	{
		/* we already have a keycode for this keysym */
		/* Does it need a shift key though ? */
		if (XKeycodeToKeysym(fk->xdpy, code, 0) != keysym)
		{
			/* TODO: Assumes 1st modifier is shifted */
			if (XKeycodeToKeysym(fk->xdpy, code, 1) == keysym)
				flags |= FAKEKEYMOD_SHIFT; /* can get at it via shift */
			else
				code = 0; /* urg, some other modifier do it the heavy way */
		}
		else
		{
			/* the keysym is unshifted; clear the shift flag if it is set */
			flags &= ~FAKEKEYMOD_SHIFT;
		}
	}

	if (!code)
	{
		int index;

		/* Change one of the last 10 keysyms to our converted utf8,
		 * remapping the x keyboard on the fly.
		 *
		 * This make assumption the last 10 arn't already used.
		 * TODO: probably safer to check for this.
		 */

		modifiedkey = (modifiedkey+1) % 10;

		/* Point at the end of keysyms, modifier 0 */

		index = (fk->max_keycode - fk->min_keycode - modifiedkey - 1) * fk->n_keysyms_per_keycode;

		fk->keysyms[index] = keysym;

		XChangeKeyboardMapping(fk->xdpy,
		                       fk->min_keycode,
		                       fk->n_keysyms_per_keycode,
		                       fk->keysyms,
		                       (fk->max_keycode-fk->min_keycode));

		XSync(fk->xdpy, False);

		/* From dasher src;
		 * There's no way whatsoever that this could ever possibly
		 * be guaranteed to work (ever), but it does.
		 *
		 */

		code = fk->max_keycode - modifiedkey - 1;

		/* The below is lightly safer;
		 *
		 * code = XKeysymToKeycode(fk->xdpy, keysym);
		 *
		 * but this appears to break in that the new mapping is not immediatly
		 * put to work. It would seem a MappingNotify event is needed so
		 * Xlib can do some changes internally ? ( xlib is doing something
		 * related to above ? )
		 *
		 * Probably better to try and grab the mapping notify *here* ?
		 */

		if (XKeycodeToKeysym(fk->xdpy, code, 0) != keysym)
		{
			/* TODO: Assumes 1st modifier is shifted */
			if (XKeycodeToKeysym(fk->xdpy, code, 1) == keysym)
				flags |= FAKEKEYMOD_SHIFT; /* can get at it via shift */
		}
	}

	if (code != 0)
	{
		fakekey_send_keyevent(fk, code, True, flags);

		fk->held_state_flags = flags;
		fk->held_keycode = code;

		return 1;
	}

	fk->held_state_flags = 0;
	fk->held_keycode = 0;

	return 0; /* failed */
}

int fakekey_press(FakeKey *fk, const unsigned char *utf8_char_in, int len_bytes, int flags)
{
	unsigned int ucs4_out;

	if (fk->held_keycode) /* key is already held down */
		return 0;

	/* TODO: check for Return key here and other chars */

	if (len_bytes < 0)
	{
		/*
		   unsigned char *p = utf8_char_in;
		   while (*p != '\0') { len_bytes++; p++; }
		 */
		len_bytes = strlen((const char*)utf8_char_in); /* OK ? */
	}

	if (utf8_to_ucs4 (utf8_char_in, &ucs4_out, len_bytes) < 1)
	{
		return 0;
	}

	/* first check for Latin-1 characters (1:1 mapping)
	   if ((keysym >= 0x0020 && keysym <= 0x007e) ||
	   (keysym >= 0x00a0 && keysym <= 0x00ff))
	   return keysym;
	 */

	if (ucs4_out > 0x00ff) /* < 0xff assume Latin-1 1:1 mapping */
		ucs4_out = ucs4_out | 0x01000000; /* This gives us the magic X keysym */

	return fakekey_press_keysym(fk, (KeySym)ucs4_out, flags);
}

void fakekey_release(FakeKey *fk)
{
	if (!fk->held_keycode)
		return;

	fakekey_send_keyevent(fk, fk->held_keycode, False, fk->held_state_flags);

	fk->held_state_flags = 0;
	fk->held_keycode = 0;
}

#endif
