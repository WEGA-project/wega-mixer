const char CSS_page[] PROGMEM = R"=====(
.button { width:105px; height:26px; font-size:0.8em; }
form {
    margin: 0 auto;
    width: 500px;
    padding: 1em;
    position:absolute;
    top: 0; 
    left: 0;
}
)=====";

const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<link rel='stylesheet' type='text/css' href='style.css'>
<form action="/rest/start">
    Status: <span id='state'>Wait</span><br>
    <p style="text-align:left;">Scales = <span id='weight'></span> <span id='timer' style="float:right;"></span></p>
    <fieldset>
      <legend id='solutionA'>A</legend>
        <p>P1 = <input type='text' name='p1'/> <span id='n1'></span><span id='r1'/></p>
        <p>P2 = <input type='text' name='p2'/> <span id='n2'></span><span id='r2'/></p>
        <p>P3 = <input type='text' name='p3'/> <span id='n3'></span><span id='r3'/></p>    
    </fieldset>    
    <fieldset>    
      <legend id='solutionB'>B</legend>
      <p>P4 = <input type='text' name='p4'/> <span id='n4'></span><span id='r4'/></p>
      <p>P5 = <input type='text' name='p5'/> <span id='n5'></span><span id='r5'/></p>    
      <p>P6 = <input type='text' name='p6'/> <span id='n6'></span><span id='r6'/></p>
      <p>P7 = <input type='text' name='p7'/> <span id='n7'></span><span id='r7'/></p>    
      <p>P8 = <input type='text' name='p8'/> <span id='n8'></span><span id='r8'/></p>
    </fieldset>    
    <input type='hidden' name='s'>
    <p id='actions'>
        <input type='submit' value='Start'/>
        <input type='button' onclick="tare();" value='Tare'/>
        <input type='button' onclick='location.href = "calibration";' value='Calibration'/>
    </p>
ver: <span id='version'></span>
</form>
<script>
function loadMeta() {
    fetch('/rest/meta').then(r => r.json()).then(r => {
        let params = new URLSearchParams(location.search);
        document.getElementById('version').textContent = r.version;
        for (let i = 1; i <= r.names.length; i++) {
            document.getElementById('n' + i).textContent = r.names[i - 1];
            let goal = params.get('p' + i);
            if (goal) {
                document.getElementsByName('p' + i)[0].value = goal;
                goalIsSet = true;
            }
        }
        document.getElementsByName('s')[0].value = params.get('s');
        const evtSource = new EventSource("/rest/events");
        evtSource.addEventListener("state",  event => onStateUpdate(JSON.parse(event.data)));
        evtSource.addEventListener("scales", event => onScalesUpdate(JSON.parse(event.data)));
        evtSource.addEventListener("report", event => onReportUpdate(JSON.parse(event.data)));
    }).catch(e => setTimeout(loadMeta, 5000));
}

function onStateUpdate(event) {
    document.getElementById('state').textContent = event.state;
    document.querySelectorAll("input").forEach(e => e.disabled = (event.state != "Ready"));
}

function onScalesUpdate(event) {
    document.getElementById('weight').textContent = event.value.toFixed(2);
}

function onReportUpdate(event) {
    document.getElementById('weight').textContent = "...";
    let goalA = event.goal[0] + event.goal[1] + event.goal[2];
    let goalB = event.goal[3] + event.goal[4] + event.goal[5] + event.goal[6] + event.goal[7];
    document.getElementById('solutionA').textContent = `A (weight: ${event.sumA.toFixed(2)} should be ${goalA.toFixed(2)})`;
    document.getElementById('solutionB').textContent = `B (weight: ${event.sumB.toFixed(2)} should be ${goalB.toFixed(2)})`;
    document.getElementById('timer').textContent = 'Timer = ' +  new Date(event.timer * 1000).toISOString().substr(11, 8);
    for (let i = 1; i <= event.goal.length; i++) {
        let goal = event.goal[i - 1];
        if (!goalIsSet) {
            document.getElementsByName('p' + i)[0].value = goal.toFixed(2);
        }
        let vol = event.result[i - 1];
        let e = document.getElementById('r' + i);
        e.textContent = vol ? '=' + vol.toFixed(2) + ' ' + (vol / goal * 100 - 100).toFixed(2) + '%' : '';
        e.style.fontWeight = event.pumpWorking == i - 1 ? 'bold' : 'normal'; 
        document.getElementById('n' + i).style.fontWeight = event.pumpWorking == i - 1 ? 'bold' : 'normal'; 
    }
}

function tare() {
    document.getElementById("weight").textContent = "Wait";
    fetch("/rest/tare")
    .then(r => {
        if (r.ok) return document.getElementById("weight").textContent = "Ok";
        throw new Error('Busy try later');
    })
    .catch(e => document.getElementById("weight").textContent = e);
}


goalIsSet = false;
loadMeta();

</script>   
)=====";

const char CALIBRATION_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<link rel='stylesheet' type='text/css' href='style.css'>
Current scale_calibration_A=<span id="curA"></span>, scale_calibration_B=<span id="curB"></span>
<ol>
    <li>Unload scales and press <input type='button' onclick="tare();" value='Set to ZERO'/> <b id='tareOk'></b></li>
    <li>Take any object of known weight. Enter its weight here <input type='text' id='knownValue'/></li>
    <li>Place the weight on point A and press <input type='button' onclick="calc('A');" value="Ok"/> <b id='A'></b></li>
    <li>Place the weight on point B and press <input type='button' onclick="calc('B');"value="Ok"/> <b id='B'></b></li>
    <li>Open <b>mixer.ino</b> and set values for the constants</li>
</ol>
Test:
<ol>
    <li>Enter scale <input type='text' id='testScale'/> and press <input type='button' onclick="test();" value='Test'/></li>
    <li>Measured weight is <span id='testVal'>_</span></li>
</ol>
<p><input type='button' onclick="location.href = '/';" value='Home'/></p>
ver : <span id='version'>unknown</span>    
<script>
function loadMeta() {
    fetch("/rest/meta").then(r => r.json()).then(r => {
        document.getElementById("version").textContent = r.version;
        document.getElementById("curA").textContent = r.scalePointA;        
        document.getElementById("curB").textContent = r.scalePointB;        
    })
    .catch(e => setTimeout(loadMeta, 1000));
}

function tare() {
    document.getElementById("tareOk").textContent = "Wait";
    fetch("/rest/tare")
    .then(r => {
        if (r.ok) return document.getElementById("tareOk").textContent = "Ok";
        throw new Error('Busy try later');
    })
    .catch(e => document.getElementById("tareOk").textContent = e);
}

function calc(point) {
    fetch("/rest/measure")
    .then(r => {
        if (r.ok) return r.json();
        throw new Error('Busy try later');
    })
    .then(r => {
        let knownValue = document.getElementById('knownValue').value;
        let result = (r.rawValue - r.rawZero) / knownValue;
        document.getElementById(point).textContent = ' scale_calibration_' + point + ' = ' + result;
    })
    .catch(e => document.getElementById(point).textContent = e);
}

function test(point) {
    fetch("/rest/measure")
    .then(r => {
        if (r.ok) return r.json();
        throw new Error('Busy try later');
    })
    .then(r => {
        let scale = document.getElementById('testScale').value;
        document.getElementById("testVal").textContent = (r.rawValue - r.rawZero) / scale;
    })
    .catch(e => document.getElementById("testVal").textContent = e);
}

loadMeta();
</script>
)=====";

const char OK_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<meta http-equiv="refresh" content="1;url=/">
Ok
)=====";

const char BUSY_page[] PROGMEM = R"=====(
<!DOCTYPE html>
Busy. Try later
)=====";

void cssPage(){
  server.send_P(200, PSTR("text/css"), CSS_page);
}
void calibrationPage(){
  if (state == STATE_READY) { 
    server.send_P(200, PSTR("text/html"), CALIBRATION_page);  
  } else {
    busyPage();
  }
}
void mainPage(){
  server.send_P(200, PSTR("text/html"), MAIN_page);
}
void okPage(){
  server.send_P(200, PSTR("text/html"), OK_page);
}
void busyPage(){
  server.send_P(307, PSTR("text/html"), BUSY_page);
}
