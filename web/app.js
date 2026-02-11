// ─── Gomoku Tournament Live Viewer ───────────────────────────────────────────
// Connects via SSE to the embedded C++ server and renders the board in real-time.

(() => {
    'use strict';

    // ─── DOM Elements ────────────────────────────────────────────────────────
    const canvas         = document.getElementById('boardCanvas');
    const ctx            = canvas.getContext('2d');
    const blackNameEl    = document.getElementById('blackName');
    const whiteNameEl    = document.getElementById('whiteName');
    const blackTimerEl   = document.getElementById('blackTimer');
    const whiteTimerEl   = document.getElementById('whiteTimer');
    const blackBarEl     = document.querySelector('.player-black');
    const whiteBarEl     = document.querySelector('.player-white');
    const progressText   = document.getElementById('progressText');
    const progressBar    = document.getElementById('progressBar');
    const lastResultEl   = document.getElementById('lastResult');
    const resultsBody    = document.getElementById('resultsBody');
    const moveListEl     = document.getElementById('moveList');
    const logContainer   = document.getElementById('logContainer');
    const statusBadge    = document.getElementById('connectionStatus');

    // ─── State ───────────────────────────────────────────────────────────────
    let currentState = null;
    let logLines     = [];
    const MAX_LOG    = 200;

    // ─── Board Drawing Constants ─────────────────────────────────────────────
    const BOARD_BG     = '#c8a96e';
    const LINE_COLOR   = '#3d2e14';
    const STAR_COLOR   = '#3d2e14';
    const COORD_COLOR  = '#5c4321';

    function columnLabel(x) {
        return String.fromCharCode(65 + x); // A, B, C, ...
    }

    // ─── Canvas Board Renderer ───────────────────────────────────────────────

    function drawBoard(board) {
        if (!board) return;

        const size   = board.size || 15;
        const dpr    = window.devicePixelRatio || 1;
        const rect   = canvas.parentElement.getBoundingClientRect();
        const w      = rect.width;
        const h      = rect.height;

        canvas.width  = w * dpr;
        canvas.height = h * dpr;
        ctx.setTransform(dpr, 0, 0, dpr, 0, 0);

        const margin   = 28;
        const gridSize = Math.min(w, h) - margin * 2;
        const cellSize = gridSize / (size - 1);
        const ox       = (w - gridSize) / 2;
        const oy       = (h - gridSize) / 2;

        // Background
        ctx.fillStyle = BOARD_BG;
        ctx.fillRect(0, 0, w, h);

        // Wood grain effect
        ctx.strokeStyle = 'rgba(90, 60, 20, 0.06)';
        ctx.lineWidth   = 1;
        for (let i = 0; i < h; i += 3) {
            ctx.beginPath();
            ctx.moveTo(0, i + Math.sin(i * 0.05) * 2);
            ctx.lineTo(w, i + Math.sin(i * 0.05 + 1) * 2);
            ctx.stroke();
        }

        // Grid lines
        ctx.strokeStyle = LINE_COLOR;
        ctx.lineWidth   = 0.8;
        for (let i = 0; i < size; i++) {
            // Vertical
            ctx.beginPath();
            ctx.moveTo(ox + i * cellSize, oy);
            ctx.lineTo(ox + i * cellSize, oy + gridSize);
            ctx.stroke();
            // Horizontal
            ctx.beginPath();
            ctx.moveTo(ox, oy + i * cellSize);
            ctx.lineTo(ox + gridSize, oy + i * cellSize);
            ctx.stroke();
        }

        // Star points
        const starPoints = getStarPoints(size);
        ctx.fillStyle = STAR_COLOR;
        for (const [sx, sy] of starPoints) {
            ctx.beginPath();
            ctx.arc(ox + sx * cellSize, oy + sy * cellSize, cellSize * 0.12, 0, Math.PI * 2);
            ctx.fill();
        }

        // Coordinates
        ctx.fillStyle = COORD_COLOR;
        ctx.font = `${Math.max(9, cellSize * 0.35)}px Inter, sans-serif`;
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        for (let i = 0; i < size; i++) {
            // Column labels (top)
            ctx.fillText(columnLabel(i), ox + i * cellSize, oy - margin * 0.5);
            // Row labels (left)
            ctx.fillText(String(size - i), ox - margin * 0.55, oy + i * cellSize);
        }

        // Stones
        const cells = board.cells || [];
        const moveOrder = board.moveOrder || [];
        for (let y = 0; y < size; y++) {
            for (let x = 0; x < size; x++) {
                const cell = cells[y * size + x];
                if (cell === 0) continue;

                const cx = ox + x * cellSize;
                const cy = oy + y * cellSize;
                const r  = cellSize * 0.43;

                drawStone(cx, cy, r, cell === 1);

                // Move number
                const moveIdx = moveOrder.findIndex(m => m[0] === x && m[1] === y);
                if (moveIdx >= 0) {
                    drawMoveNumber(cx, cy, r, moveIdx + 1, cell === 1);
                }
            }
        }

        // Last move highlight
        if (board.lastMoveX >= 0 && board.lastMoveY >= 0) {
            const lx = ox + board.lastMoveX * cellSize;
            const ly = oy + board.lastMoveY * cellSize;
            const lr = cellSize * 0.48;
            ctx.strokeStyle = '#ff4444';
            ctx.lineWidth   = 2.5;
            ctx.beginPath();
            ctx.arc(lx, ly, lr, 0, Math.PI * 2);
            ctx.stroke();
        }
    }

    function drawStone(cx, cy, r, isBlack) {
        if (isBlack) {
            // Black stone with gradient
            const grad = ctx.createRadialGradient(cx - r * 0.3, cy - r * 0.3, r * 0.1, cx, cy, r);
            grad.addColorStop(0, '#555');
            grad.addColorStop(0.6, '#1a1a2e');
            grad.addColorStop(1, '#0a0a15');
            ctx.fillStyle = grad;
        } else {
            // White stone with gradient
            const grad = ctx.createRadialGradient(cx - r * 0.3, cy - r * 0.3, r * 0.1, cx, cy, r);
            grad.addColorStop(0, '#ffffff');
            grad.addColorStop(0.7, '#e8e8e8');
            grad.addColorStop(1, '#c0c0c0');
            ctx.fillStyle = grad;
        }

        ctx.beginPath();
        ctx.arc(cx, cy, r, 0, Math.PI * 2);
        ctx.fill();

        // Subtle shadow
        ctx.shadowColor   = 'rgba(0,0,0,0.3)';
        ctx.shadowBlur    = 4;
        ctx.shadowOffsetX = 1;
        ctx.shadowOffsetY = 2;
        ctx.beginPath();
        ctx.arc(cx, cy, r, 0, Math.PI * 2);
        ctx.fill();
        ctx.shadowColor = 'transparent';
        ctx.shadowBlur  = 0;
        ctx.shadowOffsetX = 0;
        ctx.shadowOffsetY = 0;
    }

    function drawMoveNumber(cx, cy, r, num, isBlack) {
        const fontSize = Math.max(8, r * 0.9);
        ctx.font      = `600 ${fontSize}px Inter, sans-serif`;
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillStyle = isBlack ? '#ddd' : '#333';
        ctx.fillText(String(num), cx, cy + 0.5);
    }

    function getStarPoints(size) {
        if (size === 15) {
            return [[3,3],[3,7],[3,11],[7,3],[7,7],[7,11],[11,3],[11,7],[11,11]];
        }
        if (size === 19) {
            return [[3,3],[3,9],[3,15],[9,3],[9,9],[9,15],[15,3],[15,9],[15,15]];
        }
        if (size === 20) {
            return [[3,3],[3,9],[3,16],[9,3],[9,9],[9,16],[16,3],[16,9],[16,16]];
        }
        // Fallback: center point
        const mid = Math.floor(size / 2);
        return [[mid, mid]];
    }

    // ─── Timer Formatting ────────────────────────────────────────────────────

    function formatTime(ms) {
        if (ms <= 0 || ms === undefined) return '--:--';
        const totalSec = Math.floor(ms / 1000);
        const min = Math.floor(totalSec / 60);
        const sec = totalSec % 60;
        return `${min}:${sec.toString().padStart(2, '0')}`;
    }

    // ─── UI Update ───────────────────────────────────────────────────────────

    function updateUI(state) {
        if (!state) return;
        currentState = state;

        // Progress
        const completed = state.gamesCompleted || 0;
        const total     = state.gamesTotal || 0;
        progressText.textContent = `${completed} / ${total} games`;
        const pct = total > 0 ? (completed / total * 100) : 0;
        progressBar.style.width = `${pct}%`;

        // Last result
        if (state.lastResult) {
            lastResultEl.textContent = state.lastResult;
        }

        // Worker info (use first worker for the main display)
        const worker = state.workers && state.workers.length > 0 ? state.workers[0] : null;
        if (worker) {
            blackNameEl.textContent = worker.blackName || 'Black';
            whiteNameEl.textContent = worker.whiteName || 'White';
            blackTimerEl.textContent = formatTime(worker.blackTime);
            whiteTimerEl.textContent = formatTime(worker.whiteTime);

            // Active turn indicators
            if (worker.isBlackActive) {
                blackBarEl.classList.add('active');
                whiteBarEl.classList.remove('active');
            } else {
                whiteBarEl.classList.add('active');
                blackBarEl.classList.remove('active');
            }
        }

        // Board
        if (state.board) {
            drawBoard(state.board);
        }

        // Move list
        updateMoveList(state.board);

        // Results table
        updateResults(state.results);

        // Log lines (append new ones)
        if (state.logLines && state.logLines.length > 0) {
            for (const line of state.logLines) {
                if (!logLines.includes(line)) {
                    logLines.push(line);
                }
            }
            // Keep limited
            while (logLines.length > MAX_LOG) logLines.shift();
            renderLog();
        }
    }

    function updateMoveList(board) {
        if (!board || !board.moveOrder || board.moveOrder.length === 0) {
            moveListEl.innerHTML = '<span class="empty-state">Waiting for game…</span>';
            return;
        }

        let html = '';
        board.moveOrder.forEach((move, i) => {
            const label = columnLabel(move[0]) + (board.size - move[1]);
            const isBlack = (i % 2 === 0);
            const isLast = (i === board.moveOrder.length - 1);
            const cls = `move-item ${isBlack ? 'black-move' : 'white-move'} ${isLast ? 'last-move' : ''}`;
            html += `<span class="${cls}" title="Move ${i + 1}">${i + 1}.${label}</span>`;
        });
        moveListEl.innerHTML = html;

        // Auto-scroll to end
        moveListEl.scrollTop = moveListEl.scrollHeight;
    }

    function updateResults(results) {
        if (!results || results.length === 0) {
            resultsBody.innerHTML = '<tr><td colspan="6" class="empty-state">Waiting for results…</td></tr>';
            return;
        }

        let html = '';
        for (const r of results) {
            html += `<tr>
                <td>${escapeHtml(r.name1)}</td>
                <td>${escapeHtml(r.name2)}</td>
                <td class="win-cell">${r.wins}</td>
                <td class="loss-cell">${r.losses}</td>
                <td class="draw-cell">${r.draws}</td>
                <td class="score-cell">${r.score.toFixed(3)}</td>
            </tr>`;
        }
        resultsBody.innerHTML = html;
    }

    function renderLog() {
        let html = '';
        for (const line of logLines) {
            html += `<div class="log-line">${escapeHtml(line)}</div>`;
        }
        logContainer.innerHTML = html;
        logContainer.scrollTop = logContainer.scrollHeight;
    }

    function escapeHtml(str) {
        const div = document.createElement('div');
        div.textContent = str;
        return div.innerHTML;
    }

    // ─── Polling Connection ─────────────────────────────────────────────────
    // Uses HTTP polling of /api/state for maximum reliability with httplib backend.

    let pollTimer    = null;
    let pollInterval = 500;  // ms
    let failCount    = 0;

    async function poll() {
        try {
            const res  = await fetch('/api/state');
            if (!res.ok) throw new Error(`HTTP ${res.status}`);
            const data = await res.json();
            updateUI(data);
            setStatus('connected');
            failCount = 0;
        } catch {
            failCount++;
            if (failCount > 3) {
                setStatus('disconnected');
            }
        }
    }

    function startPolling() {
        setStatus('connecting');
        poll(); // immediate first poll
        pollTimer = setInterval(poll, pollInterval);
    }

    function setStatus(status) {
        statusBadge.className = 'status-badge';
        switch (status) {
            case 'connecting':
                statusBadge.textContent = 'Connecting…';
                statusBadge.classList.add('status-connecting');
                break;
            case 'connected':
                statusBadge.textContent = 'Live';
                statusBadge.classList.add('status-connected');
                break;
            case 'disconnected':
                statusBadge.textContent = 'Reconnecting…';
                statusBadge.classList.add('status-disconnected');
                break;
        }
    }

    // ─── Window Resize ───────────────────────────────────────────────────────

    let resizeTimeout;
    window.addEventListener('resize', () => {
        clearTimeout(resizeTimeout);
        resizeTimeout = setTimeout(() => {
            if (currentState && currentState.board) {
                drawBoard(currentState.board);
            }
        }, 100);
    });

    // ─── Init ────────────────────────────────────────────────────────────────

    // Draw an empty board immediately
    drawBoard({ size: 15, cells: new Array(225).fill(0), moveOrder: [], lastMoveX: -1, lastMoveY: -1 });

    // Start polling
    startPolling();

})();
