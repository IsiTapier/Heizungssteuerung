// Complete project details: https://randomnerdtutorials.com/esp32-web-server-websocket-sliders/

var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function getValues(){
    websocket.send("getValues");
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
    getValues();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function updateCheckbox(element) {
    var checkbox = element.id;
    var value = document.getElementById(element.id).checked;
    websocket.send(checkbox+value.toString());
}

function updateTimeInput(element) {
    var timeId = element.id;
    var timeValue = document.getElementById(element.id).value;
    websocket.send(timeId+timeValue.toString());
    if(timeId.substring(7,15)==="Heattime" && timeValue!="" && element.parentNode.parentNode.lastElementChild === element.parentNode) {
        addTimeInput(timeId.substring(0,7));
        element.previousElementSibling.style.visibility = "visible";
    }
}

function onMessage(event) {
    console.log(event.data);
    var myObj = JSON.parse(event.data);
    var keys = Object.keys(myObj);

    for (var i = 0; i < keys.length; i++){
        var key = keys[i];
        if(myObj[key]==="") continue;
        if(key.substring(7,15)==="Heattime") {
            while(!document.getElementById(key)) {
                addTimeInput(key.substring(0,7));
            }
            console.log(document.getElementById(key));
            document.getElementById(key).previousElementSibling.style.visibility = 'visible';
            document.getElementById(key).value = myObj[key];
            addTimeInput(key.substring(0,7)); // add one more empty one 
        }
        else if(key==="valveState"||key==="force") document.getElementById(key).checked = myObj[key];
        else document.getElementById(key).value = myObj[key];
        // document.getElementById("slider"+ (i+1).toString()).value = myObj[key];
    }
}

function addTimeInput(type) {
    console.log(type+"Heattimes");
    var container = document.getElementById(type+"Heattimes");
    var prevDiv = container.lastElementChild;
    console.log(prevDiv.lastElementChild.value);
    if(prevDiv.lastElementChild.value==="") return;
    var newDiv = prevDiv.cloneNode(true);
    var newInput = newDiv.lastElementChild;
    var number = parseInt(newInput.id.substr(15))+1;
    if(number >= 10) return;
    newInput.id = newInput.id.substr(0,15)+number;
    newInput.value = '';
    newDiv.firstElementChild.style.visibility = 'hidden';
    container.appendChild(newDiv);
}

function removeTimeInput(element) {
    var div = element.parentNode;
    var container = div.parentNode;
    if(container.children.length <= 1) return;
    // websocket.send(container.lastElementChild.lastElementChild.id+"");
    container.removeChild(div);
    var number = 0;
    // websocket.send(div.lastElementChild.id.substr(0,15)+container.children.length+"");
    var outstring = "";
    for(var child of container.children) {
        var input = child.firstElementChild.nextSibling;
        input.id = input.id.substr(0,15)+number;
        number++;
        outstring += input.id+input.value.toString();
        // websocket.send(input.id+input.value.toString());
    }
    outstring += div.lastElementChild.id.substr(0,15)+container.children.length+""; 
    websocket.send(div.lastElementChild.id.substr(0,15)+"s"+outstring);
}