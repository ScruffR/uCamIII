#pragma once

#if Wiring_WiFi
#include "WebServer.h"

#define COLOR_TYPE "CT"
#define RAW_RES    "RR"
#define JPG_RES    "JR"
#define CONTRAST   "cbeC"
#define BRIGHTNESS "cbeB"
#define EXPOSURE   "cbeE"

P(Page_start) = "<html><head><meta charset='utf-8'/><title>uCamIII Demo</title></head><body>\n";
P(camBody) = 
"<form id='settings' onclick='click'>"
"<table style='width: 100%'>"
"<tr>"
"<td width='10%'><label>Color Type</label></td>"
"<td colspan='3'>"
"<select id='" COLOR_TYPE "' name='" COLOR_TYPE "' style='width: 100%' >"
"<option value='07'>JPEG</option>"
"<option value='03'>RawGrascale8bit</option>"
"<option value='06'>RawColor16bitRGB565</option>"
"<option value='08'>RawColor16bitCrYCbY</option>"
"</select>"
"<td width='10%'><label>Contrast</label></td>"
"<td>"
"<select id='" CONTRAST "' name='" CONTRAST "' style='width: 100%' >"
"<option value='00'>Min</option>"
"<option value='01'>Low</option>"
"<option value='02' selected='true'>Default</option>"
"<option value='03'>High</option>"
"<option value='04'>Max</option>"
"</select>"
"</td>"
"<td rowspan='3'>"
"<input id='click' type='submit' value='Click!' style='width:120px; height:60px' />"
"</td>"
"</tr>"
"<tr>"
"<td width='10%'><label>Raw Resolution</label></td>"
"<td>"
"<select id='" RAW_RES "' name='" RAW_RES "' style='width: 100%' >"
"<option value='03'>160x120</option>"
"<option value='09'>128x128</option>"
"<option value='08'>128x96</option>"
"<option value='01'>80x60</option>"
"</select>"
"</td>"
"<td width='10%'><label>JPG Resolution</label></td>"
"<td>"
"<select id='" JPG_RES "' name='" JPG_RES "' style='width: 100%' >"
"<option value='07'>640x480</option>"
"<option value='05'>320x240</option>"
"<option value='03'>160x120</option>"
"</select>"
"</td>"
"<td width='10%'><label>Brightness</label></td>"
"<td>"
"<select id='" BRIGHTNESS "' name='" BRIGHTNESS "' style='width: 100%' >"
"<option value='00'>Min</option>"
"<option value='01'>Low</option>"
"<option value='02' selected='true'>Default</option>"
"<option value='03'>High</option>"
"<option value='04'>Max</option>"
"</select>"
"</td>"
"</tr>"
"<tr>"
"<td/><td/><td/><td/>"
"<td width='10%'><label>Exposure</label></td>"
"<td>"
"<select id='" EXPOSURE "' name='" EXPOSURE "' style='width: 100%' >"
"<option value='00'>-2</option>"
"<option value='01'>-1</option>"
"<option value='02' selected='true'>0</option>"
"<option value='03'>+1</option>"
"<option value='04'>+2</option>"
"</select>"
"</td>"
"</tr>"
"</table>"
"</form>"
"<br/>"
"<img src='image.dat' />"
"<br/>"
"<a href='image.dat'>download image </a>"
"<label id='timestamp'></label>"
"<br/>"
"<h3><font color='red'>16bit raw images will be displayed with distorted colors</font></h3>"
"<h4>Chrome doesn't cope well with raw images at all, better use other browser (e.g. Mozilla Firefox)</h4>"
"<br/>To view with correct colors use an image viewer that supports"
"<br/>RGB565 or CrYCbY/UYVY encoding (e.g. GIMP)"
"<br/>"
"<a href='http://www.4dsystems.com.au/productpages/uCAM-III/downloads/uCAM-III_datasheet_R_1_0.pdf'>uCamIII datasheet</a>"
"<br/><br/>"
;
P(Page_end) = "</body></html>";

P(script) = 
"<script>" 
"document.getElementById('" COLOR_TYPE "').value='%02d';"
"document.getElementById('" RAW_RES "').value='%02d';"
"document.getElementById('" JPG_RES "').value='%02d';"
"document.getElementById('" CONTRAST "').value='%02d';"
"document.getElementById('" BRIGHTNESS "').value='%02d';"
"document.getElementById('" EXPOSURE "').value='%02d';"
"document.getElementById('timestamp').innerHTML='%s%+03d:%02d';"
"</script>"
;

/* This creates an instance of the webserver.  By specifying a prefix
 * of "", all pages will be at the root of the server. */
const char PREFIX[] = "";
const int  NAMELEN  = 32;
const int  VALUELEN = 32;

WebServer webserver(PREFIX, 80);

void defaultCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete);
void imageCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete);
#endif
