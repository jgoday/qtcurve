@namespace url("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul");

/* Swap Cancel/OK -> OK/Cancel */
/*
.dialog-button-box { -moz-box-direction: reverse; -moz-box-pack: right; }
.dialog-button-box spacer { display: none !important; }
*/
/* Just the following line seems to be enough for FireFox 3 */
prefwindow { -moz-binding: url("file://@GTK_THEME_DIR@/mozilla/preferences-rev.xml#prefwindow") !important; }

