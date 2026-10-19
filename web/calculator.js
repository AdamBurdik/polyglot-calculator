
const button = document.getElementById("toggle");
const root = document.documentElement;

const saved = localStorage.getItem("theme");
if (saved === "dark" || saved === "light") {
    root.dataset.theme = saved;
}

function flipTheme() {
    let current;
    if (root.dataset.theme === "dark" || root.dataset.theme === "light") {
        current = root.dataset.theme;
    } else {
        const systemDark = window.matchMedia("(prefers-color-scheme: dark)").matches;
        current = systemDark ? "dark" : "light";
    }

    const next = current === "dark" ? "light" : "dark";
    root.dataset.theme = next;
    localStorage.setItem("theme", next);
}

button.addEventListener("click", flipTheme);




let currentInput = "";
const display = document.getElementById("display");

function render() {
    display.textContent = currentInput;
}

document.querySelectorAll("button").forEach(function (btn) {
    btn.addEventListener("click", function () {
        const label = btn.textContent.trim();
        if (label === "AC") {
            currentInput = "";
        } else if (label === "=") {
            const expr = currentInput.replace(/×/g, "*").replace(/÷/g, "/");
            fetch('/api/calc', { method: 'POST', body: expr })
                .then(function (r) { return r.json(); })
                .then(function (data) {
                    if (data.error) {
                        currentInput = data.error;
                    } else {
                        currentInput = String(data.result);
                    }
                    render();
                })
                .catch(function (err) {
                    currentInput = 'error';
                    render();
                });
            return;
        } else if (label === "π") {
            currentInput += "pi";
        } else if (label === "e") {
            currentInput += "e";
        } else {
            currentInput += label;
        }
        
        render();
    });
});