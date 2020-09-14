/*   SDLMain.m - main entry point for our Cocoa-ized SDL app
 Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
 Non-NIB-Code & other changes: Max Horn <max@quendi.de>
 
 Feel free to customize this file to suit your needs
 */

#ifndef _macsupport_h_
#define _macsupport_h_

#ifdef __cplusplus
extern "C"
{
#endif
void OSXMain();
const char *initSaveLocation();
const char* initResourcesLocation();
#ifdef __cplusplus
}
#endif

#endif /* _macsupport_h_ */
