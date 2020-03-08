class WinJS {
	constructor(elem) {
		this.windows = 0;
		this.lastIndex = 9999;
		this.minimizedWindows = [];
		this.minimizedW = 10;
		this.minimizedH = 1;

		if(elem && elem.constructor.name == "HTMLDivElement") {
			this.desk = elem;
		} else {
			throw new Error("Specified WinJS container is invalid");
		}
	}

	static makeDraggable(head, callback) {
		var win = head.parentElement;
		var p1, p2, p3, p4, x, y;
		head.onmousedown = dragMouseDown;

		function dragMouseDown(e) {
			e = e || window.event;
			e.preventDefault();
			win.focus();

			p3 = e.clientX;
			p4 = e.clientY;
			document.onmouseup = closeDragElement;
			document.onmousemove = elementDrag;
		}

		function elementDrag(e) {
			e = e || window.event;
			e.preventDefault();

			p1 = p3 - e.clientX;
			p2 = p4 - e.clientY;
			p3 = e.clientX;
			p4 = e.clientY;
			x = win.offsetLeft - p1;
			y = win.offsetTop - p2;

			if(callback) callback(x, y);
		}

		function closeDragElement() {
			document.onmouseup = null;
			document.onmousemove = null;
		}
	}

	static createElem(name, parent, type = "div") {
		var elem = document.createElement(type);
		elem.className = "winjs-" + name;
		if(parent) parent.appendChild(elem);
		return elem;
	}

	static createBtn(name, parent) {
		var elem = WinJS.createElem(name, parent);
		var icon = WinJS.createElem("i-" + name, elem);
		return { elem, icon };
	}

	static checkCallback(callback, ...params) {
		return typeof callback != "function" || callback(...params) !== false;
	}

	createWin({ title, id, icon, hidden, w, h, x, y, content, close, maximize, minimize, unmaximize, unminimize }) {
		title = title || "Untitled";
		id = id || Math.random().toString(36).slice(2);
		icon = icon || "";
		hidden = hidden || false;
		w = w || 640;
		h = h || 480;
		x = (typeof x == "number" ? x : this.desk.clientWidth / 2 - w / 2);
		y = (typeof y == "number" ? y : this.desk.clientHeight / 2 - h / 2);
		content = content || "";

		if(document.querySelector("#" + id)) {
			throw new Error("Element with specified ID already exists");
		}

		var self = this;

		var win = WinJS.createElem("window");
		win.id = id;
		win.hidden = hidden;
		win.tabIndex = 0;
		win.style.zIndex = ++this.lastIndex;
		win.style.left = x + "px";
		win.style.top = y + "px";

		win.addEventListener("focusin", function() {
			// Set window z-index
			self.lastIndex += self.windows;
			win.style.zIndex = self.lastIndex;
		}, false);
		win.addEventListener("focusout", function() {
			// Reset z-index if it's too big
			if(self.lastIndex > 1000000000) self.lastIndex = 9999 + self.windows;
		}, false);

		var maximized = false;
		var minimized = false;
		var oldX, oldY;

		// Helper functions
		var setPos = function(nx, ny) {
			if(nx != undefined) {
				x = nx;
				win.style.left = nx + "px";
			}
			if(ny != undefined) {
				y = ny;
				win.style.top = ny + "px";
			}
		};
		var setSize = function(nw, nh) {
			if(nw != undefined) {
				w = nw;
				body.style.width = nw + "px";
			}
			if(nh != undefined) {
				h = nh;
				body.style.height = nh + "px";
			}
		};

		var head = WinJS.createElem("header", win);
		WinJS.makeDraggable(head, function(nx, ny) {
			// Update window position
			setPos(nx, ny);

			// If window is minimized and it's in pseudo task bar
			var i = self.minimizedWindows.indexOf(id);
			if(minimized && i != -1) {
				// Remove window from the pseudo task bar
				if(i == self.minimizedWindows.length - 1) {
					// Window is at the end, just remove it from the list
					self.minimizedWindows.pop();
				} else {
					// Window is not at the end, remove it from the list...
					self.minimizedWindows.splice(i, 1);

					// ...and re-arrange windows after that window
					var after = self.minimizedWindows.slice(i);
					for(var i = 0, l = after.length; i < l; i++) {
						var e = document.querySelector("#" + after[i]);
						e.style.left = Number(e.style.left.slice(0, -2)) - (10 + win.offsetWidth) + "px";
					}
				}
				self.minimizedW -= 10 + win.offsetWidth;
			}
		});

		var iconElem = WinJS.createElem("icon", head, "img");
		iconElem.src = icon;

		var titleElem = WinJS.createElem("title", head);
		titleElem.innerHTML = title;

		var buttons = WinJS.createElem("buttons", head);

		if(minimize) var { elem: minimizeBtn, icon: minimizeIcon } = WinJS.createBtn("minimize", buttons);
		if(maximize) var { elem: maximizeBtn, icon: maximizeIcon } = WinJS.createBtn("maximize", buttons);
		if(close) var { elem: closeBtn, icon: closeIcon } = WinJS.createBtn("close", buttons);

		var body = WinJS.createElem("content", win);
		body.style.width = w + "px";
		body.style.height = h + "px";
		body.innerHTML = content;

		// Callbacks
		var closeFunc = function() {
			if(WinJS.checkCallback(close)) {
				// Hide window
				win.hidden = true;
			}
		};
		var maximizeFunc = function() {
			if(WinJS.checkCallback(maximize, maximized, win)) {
				if(!maximized) {
					// Save window position
					oldX = x;
					oldY = y;

					// Change window position
					setPos(0, 0);

					// Change inner window size
					// 3 = border size, 31 = heading size
					body.style.width = self.desk.clientWidth - 3 + "px";
					body.style.height = self.desk.clientHeight - 3 - 31 + "px";

					// Change maximize button icon
					maximizeIcon.className = "winjs-i-unmaximize";
				} else {
					// Change window position
					setPos(oldX, oldY);

					// Change inner window size
					body.style.width = w + "px";
					body.style.height = h + "px";

					// Change maximize button icon
					maximizeIcon.className = "winjs-i-maximize";
				}
				maximized = !maximized;
			}
		};
		var minimizeFunc = function() {
			if(WinJS.checkCallback(minimize, minimized, win)) {
				if(!minimized) {
					// Change window properties to make it look like minimized window
					body.hidden = true;
					if(maximize) maximizeBtn.style.display = "none";
					minimizeIcon.className = "winjs-i-unminimize";

					// Move pseudo task bar higher when it goes outside the container
					if(self.minimizedW + 10 + win.offsetWidth > self.desk.clientWidth) {
						// Do some magic here
					}

					// Save window position
					oldX = x;
					oldY = y;

					// Change window position
					setPos(self.minimizedW, self.desk.clientHeight - ((31 + 10) * self.minimizedH))

					// Add window to pseudo task bar
					self.minimizedWindows.push(id);
					self.minimizedW += 10 + win.offsetWidth;
				} else {
					// Change window position
					setPos(oldX, oldY);

					// Remove window from the pseudo task bar
					var i = self.minimizedWindows.indexOf(id);
					if(i != -1) {
						if(i == self.minimizedWindows.length - 1) {
							// Window is at the end, just remove it from the list
							self.minimizedWindows.pop();
						} else {
							// Window is not at the end, remove it from the list...
							self.minimizedWindows.splice(i, 1);

							// ...and re-arrange windows after that window
							var after = self.minimizedWindows.slice(i);
							for(var i = 0, l = after.length; i < l; i++) {
								var el = document.querySelector("#" + after[i]);
								el.style.left = Number(el.style.left.slice(0, -2)) - (10 + win.offsetWidth) + "px";
							}
						}
						self.minimizedW -= 10 + win.offsetWidth;
					}

					// Change window properties to restore it
					minimizeIcon.className = "winjs-i-minimize";
					if(maximize) maximizeBtn.style.display = "";
					body.hidden = false;
				}
				minimized = !minimized;
			}
		};

		if(close) closeBtn.addEventListener("click", closeFunc, false);
		if(maximize) maximizeBtn.addEventListener("click", maximizeFunc, false);
		if(minimize) minimizeBtn.addEventListener("click", minimizeFunc, false);

		this.windows++;
		this.desk.appendChild(win);

		return {
			main: win, body,
			setPos, setSize,
			close: closeFunc,
			maximize: maximizeFunc,
			minimize: minimizeFunc
		};
	}
}
