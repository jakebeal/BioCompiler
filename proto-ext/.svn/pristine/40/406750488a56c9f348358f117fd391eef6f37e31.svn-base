function setCookie(c_name,value,exdays) {
    var exdate=new Date();
    exdate.setDate(exdate.getDate() + exdays);
    var c_value=escape(value) + ((exdays==null) ? "" : "; expires="+exdate.toUTCString());
    var data = c_name + "=" + c_value;
    document.cookie = data;
}

function getCookie(c_name) {
    var c_value = document.cookie;
    var c_start = c_value.indexOf(" " + c_name + "=");
    if (c_start == -1) {
	c_start = c_value.indexOf(c_name + "=");
    }
    if (c_start == -1) {
	c_value = null;
    } else {
	c_start = c_value.indexOf("=", c_start) + 1;
	var c_end = c_value.indexOf(";", c_start);
	if (c_end == -1)
	{
	    c_end = c_value.length;
	}
	c_value = unescape(c_value.substring(c_start,c_end));
    }
    return c_value;
}

function tour_program() {
    var prg = 
        ";; Here's an example program to help\n;; you get started...\n" +
	"(let (;; Measure elapsed time\n" +
        "      (time (timer))\n" +
	"      ;; Measure distance device 0\n" +
	"      (phase (distance-to (= (mid) 0))))\n" +
	"  ;; Add distance & time for a shifting pattern\n" + 
	"  (let ((wave (sin (* 0.1 (- time phase)))))\n" +
	"    ;; Display scaled pattern with green LED\n" + 
	"    (green (+ 0.5 (* 0.5 wave)))))\n";
    return prg;
}

function maybe_run_tour() {
    var do_tour = getCookie('tour');
    if(do_tour == 'yes' || do_tour == null) {
	introJs().start();
	setCookie('tour','no',20*365)
    }
}
