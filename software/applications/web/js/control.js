//@ts-check
window.onload = function init() {
    var elem = document.getElementById('stream');
    var ws = new WebSocket("ws://" + location.host);
    
    // @ts-ignore
    elem.src = "http://" + location.host + ":8088/?action=stream";

    ws.onopen = function(ev)  { console.log(ev); };
    ws.onerror = function(ev) { console.log(ev); };
    ws.onclose = function(ev) { console.log(ev); };
    ws.onmessage = function (ev) {
        console.log(ev);
    };

    document.addEventListener("keydown",
    function (ev) {
        var KEY_LEFT = 37,
            KEY_UP = 38,
            KEY_RIGHT = 39,
            KEY_DOWN = 40,
            KEY_TAB = 9,
            KEY_ENTER = 13,
            KEY_BACKSPACE = 8,
            KEY_DELETE = 46;
        // @ts-ignore
        var key = ev.keyCode;
    
        if (key == KEY_LEFT) {
            ws.send("L");
            ev.preventDefault();
            console.log("←");
        }
        else if (key == KEY_UP) {
            ws.send("U");
            ev.preventDefault();
            console.log("↑");
        }
        else if (key == KEY_RIGHT) {
            ws.send("R");
            ev.preventDefault();
            console.log("→");
        }
        else if (key == KEY_DOWN) {
            ws.send("D");
            ev.preventDefault();
            console.log("↓");
        }
    });
};
