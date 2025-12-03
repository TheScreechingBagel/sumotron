var targetUrl = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener("load", onLoad);

function onLoad() {
  initializeSocket();
}

function initializeSocket() {
  console.log("Opening WebSocket connection to ESP32...");
  websocket = new WebSocket(targetUrl);
  websocket.onopen = onOpen;
  websocket.onclose = onClose;
  websocket.onmessage = onMessage;
}
function onOpen(event) {
  console.log("Starting connection to server..");
}
function onClose(event) {
  console.log("Closing connection to server..");
  setTimeout(initializeSocket, 2000);
}
function onMessage(event) {
  console.log("WebSocket message received:", event);
}

function sendMessage(message) {
  websocket.send(message);
}

/*
Speed Settings Handler
*/
var speedSettings = document.querySelectorAll(
  'input[type=radio][name="speed-settings"]',
);
speedSettings.forEach((radio) =>
  radio.addEventListener("change", () => {
    var speedSettings = radio.value;
    console.log("Speed Settings :: " + speedSettings);
    sendMessage(speedSettings);
  }),
);

/*
O-Pad/ D-Pad Controller and Javascript Code
*/
// Prevent scrolling on every click!
// super sweet vanilla JS delegated event handling!
document.body.addEventListener("click", function (e) {
  if (e.target && e.target.nodeName == "A") {
    e.preventDefault();
  }
});

function touchStartHandler(event) {
  var direction = event.target.dataset.direction;
  console.log("Touch Start :: " + direction);
  sendMessage(direction);
}

function touchEndHandler(event) {
  const stop_command = "stop";
  var direction = event.target.dataset.direction;
  console.log("Touch End :: " + direction);
  sendMessage(stop_command);
}

document.querySelectorAll(".control").forEach((item) => {
  item.addEventListener("touchstart", touchStartHandler);
});

document.querySelectorAll(".control").forEach((item) => {
  item.addEventListener("touchend", touchEndHandler);
});

/*
Mode Switcher
*/
const btnClassic = document.getElementById("btn-classic");
const btnSumo = document.getElementById("btn-sumo");
const modeClassic = document.getElementById("mode-classic");
const modeSumo = document.getElementById("mode-sumo");

if (btnClassic && btnSumo && modeClassic && modeSumo) {
  btnClassic.addEventListener("click", () => {
    btnClassic.classList.add("active");
    btnSumo.classList.remove("active");
    modeClassic.classList.add("active");
    modeSumo.classList.remove("active");
  });

  // Request fullscreen on Sumo button click
  btnSumo.addEventListener("click", () => {
    btnSumo.classList.add("active");
    btnClassic.classList.remove("active");
    modeSumo.classList.add("active");
    modeClassic.classList.remove("active");

    if (document.documentElement.requestFullscreen) {
      document.documentElement.requestFullscreen().catch((e) => console.log(e));
    }
  });

  // Initialize Sumo as active (logic matches HTML)
  // No extra code needed as HTML has 'active' class, but good to be explicit if we want to force it.
}

/*
Sumo Mode Sliders
*/
const sliderLeft = document.getElementById("slider-left");
const sliderRight = document.getElementById("slider-right");

function handleSliderChange() {
  const leftVal = parseInt(sliderLeft.value);
  const rightVal = parseInt(sliderRight.value);

  const leftSpeed = mapSpeed(leftVal);
  const rightSpeed = mapSpeed(rightVal);

  const command = `M:${leftSpeed},${rightSpeed}`;
  console.log("Sumo Command:", command);
  sendMessage(command);
}

function mapSpeed(val) {
  const deadzone = 20;
  const minSpeed = 100;
  const maxSpeed = 255;

  if (Math.abs(val) < deadzone) {
    return 0;
  }

  // Map [deadzone, 255] to [minSpeed, maxSpeed]
  // and [-255, -deadzone] to [-maxSpeed, -minSpeed]
  if (val > 0) {
    return Math.round(
      minSpeed + ((val - deadzone) * (maxSpeed - minSpeed)) / (255 - deadzone),
    );
  } else {
    return Math.round(
      -minSpeed + ((val + deadzone) * (maxSpeed - minSpeed)) / (255 - deadzone),
    );
  }
}

if (sliderLeft && sliderRight) {
  sliderLeft.addEventListener("input", handleSliderChange);
  sliderRight.addEventListener("input", handleSliderChange);

  // Reset sliders on release (optional, but good for safety)
  sliderLeft.addEventListener("change", () => {
    sliderLeft.value = 0;
    handleSliderChange();
  });
  sliderRight.addEventListener("change", () => {
    sliderRight.value = 0;
    handleSliderChange();
  });
}
