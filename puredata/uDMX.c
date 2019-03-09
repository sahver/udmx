
// https://github.com/pure-data/externals-howto

/*
	udmx.c

	pd-Interface to the [ a n y m a | udmx - Open Source USB Sensor Box ]

	Authors:	Michael Egger
	Copyright:	2007 [ a n y m a ]
	Website:	www.anyma.ch

	License:	GNU GPL 2.0 www.gnu.org

	Version:	0.3	2019-03
			0.2	2009-06-30
			0.1	2007-01-28
	*/

// import standard libc API
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Import the API for Pd externals.  This is provided by the Pd installation,
// and this may require manually configuring the Pd include path in the
// Makefile.
#include "m_pd.h"

// uDMX commands
#include "../common/uDMX_cmds.h"

// Import libusb
#include "usb.h"

// uDMX device & vendor
#define USBDEV_SHARED_VENDOR    0x16C0  /* VOTI */
#define USBDEV_SHARED_PRODUCT   0x05DC  /* Obdev's free shared PID */

//--------------------------------------------------------------------------

static t_class *uDMX_class;		// global pointer to the object class - so max can reference the object

typedef struct _uDMX			// defines our object's internal variables for each instance in a patch
{
	t_object x_obj;			// object header - ALL objects MUST begin with this...
	t_inlet *x_in2;

	usb_dev_handle *dev;		// handle to the udmx usb device

	float channel;			// int value - received from the right inlet and stored internally for each object instance

	int debug;
} t_uDMX;

//--------------------------------------------------------------------------

static int usbGetStringAscii(usb_dev_handle *dev, int index, int langid, char *buf, int buflen)
{
	char    buffer[256];
	int     rval, i;

	if((rval = usb_control_msg(dev, USB_ENDPOINT_IN, USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) + index, langid, buffer, sizeof(buffer), 1000)) < 0) {
		return rval;
	}


	if(buffer[1] != USB_DT_STRING) {
		return 0;
	}

    	if((unsigned char)buffer[0] < rval) {
	        rval = (unsigned char)buffer[0];
	}

	rval /= 2;

	// lossy conversion to ISO Latin1
	for(i=1;i<rval;i++) {
		if(i > buflen) {  //destination buffer overflow
            		break;
		}
		buf[i-1] = buffer[2 * i];
		if(buffer[2 * i + 1] != 0) {  // outside of ISO Latin1 range
			buf[i-1] = '?';
		}
	}

	buf[i-1] = 0;
    	return i-1;
}

//--------------------------------------------------------------------------
void find_device(t_uDMX *x)
{
	usb_dev_handle      *handle = NULL;

	struct usb_bus      *bus;
	struct usb_device   *dev;

 	usb_init();
	usb_find_busses();
	usb_find_devices();

	for(bus=usb_get_busses(); bus; bus=bus->next) {
		for(dev=bus->devices; dev; dev=dev->next) {
			if(dev->descriptor.idVendor == USBDEV_SHARED_VENDOR && dev->descriptor.idProduct == USBDEV_SHARED_PRODUCT) {

				char    string[256];
				int     len;

				handle = usb_open(dev); // we need to open the device in order to query strings

				if (!handle) {
					error ("uDMX: Warning: cannot open USB device: %s", usb_strerror());
					continue;
				}

				// now find out whether the device actually is udmx
				len = usbGetStringAscii(handle, dev->descriptor.iManufacturer, 0x0409, string, sizeof(string));

				if(len < 0) {
					post("uDMX: warning: cannot query manufacturer for device: %s", usb_strerror());
					goto skipDevice;
				}

				//post("uDMX: seen device from vendor ->%s<-", string);
				if(strcmp(string, "www.anyma.ch") != 0) {
					post("uDMX: warning: iManufacturer is not 'www.anymaa.ch'");
					goto skipDevice;
				}

				len = usbGetStringAscii(handle, dev->descriptor.iProduct, 0x0409, string, sizeof(string));

				if(len < 0) {
					post("uDMX: warning: cannot query product for device: %s", usb_strerror());
					goto skipDevice;
				}

				//post("uDMX: seen product ->%s<-", string);
				if(strcasecmp(string, "udmx") == 0) {
					break;
				}

				skipDevice:
					post("uDMX: warning: skipping the device");
					usb_close(handle);
					handle = NULL;
			}
		}

		if (handle) {
			break;
		}
	}

	if(!handle) {
		post("uDMX: Could not find USB device www.anyma.ch/udmx");
		x->dev = NULL;
	} else {
		x->dev = handle;
		post("uDMX: Found USB device www.anyma.ch/udmx");
	}

}
//--------------------------------------------------------------------------

void uDMX_float(t_uDMX *x, t_floatarg f)
{
	if (f > 255) f=255;
	if (f < 0) f=0;
	if (x->channel > 512) x->channel=512;
	if (x->channel < 0) x->channel=0;

/*
	if (x->debug) {
		post("uDMX: channel %i set to %i", (int) x->channel, (int) f);
	}
*/

	unsigned char       buffer[8];
	int                 nBytes;

	if (!(x->dev)) find_device(x);
	else {
		nBytes = usb_control_msg(x->dev, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
									cmd_SetSingleChannel, (int) f, (int) x->channel, buffer, sizeof(buffer), 5000);
		if(nBytes < 0)
			 if (x->debug) error("uDMX: USB error: %s", usb_strerror());
	}

}

//--------------------------------------------------------------------------

void uDMX_list(t_uDMX *x, t_symbol *s, short ac, t_atom *av)
{
	int i;
	unsigned char* 	buf = malloc(ac);
	int           	nBytes;
	int 		n;

	if (x->channel > 512) x->channel=512;
	if (x->channel < 0) x->channel=0;

	if (!(x->dev)) find_device(x);
	else {

		if (x->debug) post("uDMX: ac: %i\n", ac);
		for(i=0; i<ac; ++i,av++) {
			if (av->a_type==A_FLOAT) {
				n = (int) av->a_w.w_float;
				if (n > 255) n=255;
				if (n < 0) n=0;

				buf[i] = n;
			} else
				buf[i] = 0;
		}
		nBytes = usb_control_msg(x->dev, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
									cmd_SetChannelRange, ac, x->channel, buf, ac, 5000);
		if (x->debug) post( "bytes returned: %i\n", nBytes);
		if(nBytes < 0)
           if (x->debug) error("uDMX: USB error: %s\n", usb_strerror());
		else if(nBytes > 0)  if (x->debug) post("uDMX: returned: %i\n", (int)(buf[0]));
		free(buf);
	}
}

//--------------------------------------------------------------------------

void uDMX_debug(t_uDMX *x)
{
	x->debug = ! x->debug;
	post("uDMX: debug=%i", x->debug);
}

//--------------------------------------------------------------------------

void uDMX_free(t_uDMX *x)
{
	if (x->dev) {
		usb_close(x->dev);
	}
}

//--------------------------------------------------------------------------

void uDMX_open(t_uDMX *x)
{
	if (x->dev) {
		post("uDMX: There is already a connection to www.anyma.ch/udmx",0);
	} else find_device(x);
}

//--------------------------------------------------------------------------

void uDMX_close(t_uDMX *x)
{
	if (x->dev) {
		usb_close(x->dev);
		x->dev = NULL;
		post("uDMX: Closed connection to www.anyma.ch/udmx",0);
	} else
		post("uDMX: There was no open connection to www.anyma.ch/udmx",0);
}


//--------------------------------------------------------------------------

void *uDMX_new(t_floatarg ch)
{
	t_uDMX *x = (t_uDMX *)pd_new(uDMX_class);

	x->channel = ch;
	x->debug = 1;
	x->dev = NULL;

	// 2nd inlet: set channel
	x->x_in2 = floatinlet_new (&x->x_obj, &x->channel);

	find_device(x);

	return (void *) x;
}

//--------------------------------------------------------------------------

void uDMX_setup(void)
{
	uDMX_class = class_new ( gensym("uDMX"),
				 (t_newmethod) uDMX_new,
				 0, sizeof(t_uDMX),
				 CLASS_DEFAULT,
				 A_DEFFLOAT, 0);

	class_addfloat ( uDMX_class, (t_method) uDMX_float); 
	class_addmethod( uDMX_class, (t_method) uDMX_debug, gensym("debug"), 0);
	class_addlist  ( uDMX_class, (t_method) uDMX_list);
	class_addmethod( uDMX_class, (t_method) uDMX_open, gensym("open"), 0);
	class_addmethod( uDMX_class, (t_method) uDMX_close, gensym("close"), 0);

	post("\nuDMX version 0.9 - (c) 2006 [ a n y m a ]", 0);
}

//--------------------------------------------------------------------------
