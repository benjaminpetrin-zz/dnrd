<?

function menu($entry) {

  $urls = array("Home"          => "index.php",
		"Download"      => "download.php",
		"Manual Page" => "docs.php",
		"Project Page"  => "http://sourceforge.net/projects/dnrd",
		"Contact Info"  => "contact.php",
		"Links" => "links.php"
		);


  print ('<div id="navigation">');
  print ("<dl>\n");

  foreach ($urls as $i => $u) {
    print ( "<dt>");
    print( ($entry == $i) ?
	   "<b>$i</b>" : "<a href=\"$u\">$i</a>");
    print("</dt>\n");
  }
  print ("</dl>\n");
?>

<a href="http://sourceforge.net"><img src="http://sourceforge.net/sflogo.php?group_id=72&amp;type=1" border="0" alt="SourceForge.net Logo" /></a>

<?
  print ("</div>\n");
}
?>
