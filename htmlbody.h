char htmlBody[] = "<!DOCTYPE html>\
<html> <body> <center>\
<h1>Set the color of LED light:<br></h1>\
<form action=\"\" method=\"get\">\
<p style=\"color:black;font-size:28px\"> LED color:<br> </p>\
<p style=\"color:red;font-size:24px\">\
<label for=\"red\">Red</label>\
<input type=\"checkbox\" name=\"red\"> </p>\
<p style=\"color:green;font-size:24px\">\
<label for=\"green\">Green</label>\
<input type=\"checkbox\" name=\"green\"> </p>\
<p style=\"color:blue;font-size:24px\">\
<label for=\"blue\">Blue</label>\
<input type=\"checkbox\" name=\"blue\"> </p>\
<p style=\"color:black;font-size:28px\">Turn Light:\
<input type=\"radio\" name=\"OnOff\" value=\"On\"> On\
<input type=\"radio\" name=\"OnOff\" value=\"Off\" checked> Off<br></p>\
<fieldset>\
<legend style=\"color:black;font-size:28px\">Environmental variables: </legend>\
<p style=\"color:black;font-size:28px\">\
Temperature: %.2d<br>\
Humidity: %.2d<br>\
Pressure: %.3f in Hg<br></p>\
</fieldset> <br>\
<input type=\"submit\" value=\"Submit\">\
</form> </center> </body> </html>\r\n\r\n";
