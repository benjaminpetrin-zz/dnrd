<?

function menu($entry) {

  $urls = array("Home"          => "index.php",
		"Download"      => "download.php",
		"Documentation" => "docs.php",
		"Project Page"  => "http://sourceforge.net/projects/dnrd",
		"Contact Info"  => "contact.php"
		);


  print ('<div id="navigation">');
  print ("<dl>\n");

  foreach ($urls as $i => $u) {
    print ( "<dt>");
    print( ($entry == $i) ?
	   "<h3>$i</h3>" : "<a href=\"$u\">$i</a>");
    print("</dt>\n");
  }
  print ("</dl></div>\n");
}
?>
