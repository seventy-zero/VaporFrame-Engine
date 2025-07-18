/**
 * VaporFrame UI Bridge
 * Handles bidirectional communication between C++ engine and JavaScript UI
 */

class VaporFrameUI {
    constructor() {
        this.callbacks = new Map();
        this.eventListeners = new Map();
        this.isInitialized = false;
        this.engineVersion = '0.1.0';
        
        this.setupEventListeners();
        this.initialize();
    }
    
    /**
     * Initialize the UI bridge
     */
    initialize() {
        console.log('VaporFrame UI Bridge initializing...');
        
        // Set up global window object for C++ to call
        window.vaporframe = {
            call: (method, data) => this.handleEngineCall(method, data),
            version: this.engineVersion
        };
        
        // Notify engine that UI is ready
        this.callEngine('uiReady', {
            version: this.engineVersion,
            timestamp: Date.now()
        });
        
        this.isInitialized = true;
        console.log('VaporFrame UI Bridge initialized successfully');
    }
    
    /**
     * Set up event listeners for UI interactions
     */
    setupEventListeners() {
        // Handle window resize
        window.addEventListener('resize', () => {
            this.callEngine('windowResize', {
                width: window.innerWidth,
                height: window.innerHeight
            });
        });
        
        // Handle visibility change
        document.addEventListener('visibilitychange', () => {
            this.callEngine('visibilityChange', {
                visible: !document.hidden
            });
        });
        
        // Handle beforeunload
        window.addEventListener('beforeunload', () => {
            this.callEngine('uiUnload', {});
        });
    }
    
    /**
     * Call C++ engine functions from JavaScript
     * @param {string} method - Method name to call
     * @param {object} data - Data to pass to the engine
     */
    callEngine(method, data) {
        if (!this.isInitialized) {
            console.warn('UI Bridge not initialized, cannot call engine');
            return;
        }
        
        try {
            // This will be handled by the WebView implementation
            if (window.vaporframe && window.vaporframe.call) {
                window.vaporframe.call(method, JSON.stringify(data));
            } else {
                console.log(`[Engine Call] ${method}:`, data);
            }
        } catch (error) {
            console.error('Error calling engine:', error);
        }
    }
    
    /**
     * Register JavaScript callbacks for C++ to call
     * @param {string} name - Callback name
     * @param {function} callback - Callback function
     */
    registerCallback(name, callback) {
        this.callbacks.set(name, callback);
        console.log(`Registered callback: ${name}`);
    }
    
    /**
     * Handle calls from C++ engine to JavaScript
     * @param {string} method - Method name
     * @param {string} data - JSON string data
     */
    handleEngineCall(method, data) {
        try {
            const parsedData = JSON.parse(data);
            const callback = this.callbacks.get(method);
            
            if (callback) {
                callback(parsedData);
            } else {
                console.warn(`No callback registered for method: ${method}`);
            }
        } catch (error) {
            console.error('Error handling engine call:', error);
        }
    }
    
    /**
     * Add event listener for UI events
     * @param {string} event - Event name
     * @param {function} listener - Event listener function
     */
    addEventListener(event, listener) {
        if (!this.eventListeners.has(event)) {
            this.eventListeners.set(event, []);
        }
        this.eventListeners.get(event).push(listener);
    }
    
    /**
     * Remove event listener
     * @param {string} event - Event name
     * @param {function} listener - Event listener function
     */
    removeEventListener(event, listener) {
        if (this.eventListeners.has(event)) {
            const listeners = this.eventListeners.get(event);
            const index = listeners.indexOf(listener);
            if (index > -1) {
                listeners.splice(index, 1);
            }
        }
    }
    
    /**
     * Emit event to registered listeners
     * @param {string} event - Event name
     * @param {object} data - Event data
     */
    emitEvent(event, data) {
        if (this.eventListeners.has(event)) {
            this.eventListeners.get(event).forEach(listener => {
                try {
                    listener(data);
                } catch (error) {
                    console.error(`Error in event listener for ${event}:`, error);
                }
            });
        }
    }
    
    /**
     * UI component helpers
     */
    
    /**
     * Create a button element
     * @param {string} text - Button text
     * @param {function} onClick - Click handler
     * @param {string} className - Additional CSS classes
     * @returns {HTMLElement} Button element
     */
    createButton(text, onClick, className = '') {
        const button = document.createElement('button');
        button.className = `vf-button ${className}`;
        button.textContent = text;
        button.addEventListener('click', onClick);
        return button;
    }
    
    /**
     * Create a panel element
     * @param {string} title - Panel title
     * @param {string|HTMLElement} content - Panel content
     * @param {string} className - Additional CSS classes
     * @returns {HTMLElement} Panel element
     */
    createPanel(title, content, className = '') {
        const panel = document.createElement('div');
        panel.className = `vf-panel ${className}`;
        
        const header = document.createElement('div');
        header.className = 'vf-panel-header';
        
        const titleEl = document.createElement('h3');
        titleEl.className = 'vf-panel-title';
        titleEl.textContent = title;
        
        header.appendChild(titleEl);
        panel.appendChild(header);
        
        const contentEl = document.createElement('div');
        contentEl.className = 'vf-panel-content';
        
        if (typeof content === 'string') {
            contentEl.innerHTML = content;
        } else {
            contentEl.appendChild(content);
        }
        
        panel.appendChild(contentEl);
        return panel;
    }
    
    /**
     * Create a card element
     * @param {string|HTMLElement} content - Card content
     * @param {string} className - Additional CSS classes
     * @returns {HTMLElement} Card element
     */
    createCard(content, className = '') {
        const card = document.createElement('div');
        card.className = `vf-card ${className}`;
        
        if (typeof content === 'string') {
            card.innerHTML = content;
        } else {
            card.appendChild(content);
        }
        
        return card;
    }
    
    /**
     * Create an input element
     * @param {string} type - Input type
     * @param {string} placeholder - Placeholder text
     * @param {function} onChange - Change handler
     * @param {string} className - Additional CSS classes
     * @returns {HTMLElement} Input element
     */
    createInput(type, placeholder, onChange, className = '') {
        const input = document.createElement('input');
        input.type = type;
        input.className = `vf-input ${className}`;
        input.placeholder = placeholder;
        input.addEventListener('input', onChange);
        return input;
    }
    
    /**
     * Create a select element
     * @param {Array} options - Array of option objects {value, text}
     * @param {function} onChange - Change handler
     * @param {string} className - Additional CSS classes
     * @returns {HTMLElement} Select element
     */
    createSelect(options, onChange, className = '') {
        const select = document.createElement('select');
        select.className = `vf-select ${className}`;
        select.addEventListener('change', onChange);
        
        options.forEach(option => {
            const optionEl = document.createElement('option');
            optionEl.value = option.value;
            optionEl.textContent = option.text;
            select.appendChild(optionEl);
        });
        
        return select;
    }
    
    /**
     * Create a checkbox element
     * @param {string} label - Checkbox label
     * @param {boolean} checked - Initial checked state
     * @param {function} onChange - Change handler
     * @param {string} className - Additional CSS classes
     * @returns {HTMLElement} Checkbox container element
     */
    createCheckbox(label, checked, onChange, className = '') {
        const container = document.createElement('label');
        container.className = `vf-checkbox ${className}`;
        
        const input = document.createElement('input');
        input.type = 'checkbox';
        input.checked = checked;
        input.addEventListener('change', onChange);
        
        const labelEl = document.createElement('span');
        labelEl.textContent = label;
        
        container.appendChild(input);
        container.appendChild(labelEl);
        
        return container;
    }
    
    /**
     * Create a slider element
     * @param {number} min - Minimum value
     * @param {number} max - Maximum value
     * @param {number} value - Initial value
     * @param {function} onChange - Change handler
     * @param {string} className - Additional CSS classes
     * @returns {HTMLElement} Slider element
     */
    createSlider(min, max, value, onChange, className = '') {
        const slider = document.createElement('input');
        slider.type = 'range';
        slider.className = `vf-slider ${className}`;
        slider.min = min;
        slider.max = max;
        slider.value = value;
        slider.addEventListener('input', onChange);
        return slider;
    }
    
    /**
     * Create a progress bar element
     * @param {number} value - Progress value (0-100)
     * @param {string} className - Additional CSS classes
     * @returns {HTMLElement} Progress bar element
     */
    createProgressBar(value, className = '') {
        const container = document.createElement('div');
        container.className = `vf-progress ${className}`;
        
        const bar = document.createElement('div');
        bar.className = 'vf-progress-bar';
        bar.style.width = `${Math.max(0, Math.min(100, value))}%`;
        
        container.appendChild(bar);
        return container;
    }
    
    /**
     * Create a badge element
     * @param {string} text - Badge text
     * @param {string} type - Badge type (primary, success, warning, error)
     * @param {string} className - Additional CSS classes
     * @returns {HTMLElement} Badge element
     */
    createBadge(text, type = 'primary', className = '') {
        const badge = document.createElement('span');
        badge.className = `vf-badge vf-badge-${type} ${className}`;
        badge.textContent = text;
        return badge;
    }
    
    /**
     * Utility functions
     */
    
    /**
     * Format bytes to human readable string
     * @param {number} bytes - Number of bytes
     * @returns {string} Formatted string
     */
    formatBytes(bytes) {
        if (bytes === 0) return '0 B';
        const k = 1024;
        const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    }
    
    /**
     * Format time to human readable string
     * @param {number} seconds - Time in seconds
     * @returns {string} Formatted string
     */
    formatTime(seconds) {
        if (seconds < 1) {
            return `${(seconds * 1000).toFixed(1)} ms`;
        } else if (seconds < 60) {
            return `${seconds.toFixed(2)} s`;
        } else {
            const minutes = Math.floor(seconds / 60);
            const remainingSeconds = seconds % 60;
            return `${minutes}m ${remainingSeconds.toFixed(0)}s`;
        }
    }
    
    /**
     * Debounce function
     * @param {function} func - Function to debounce
     * @param {number} wait - Wait time in milliseconds
     * @returns {function} Debounced function
     */
    debounce(func, wait) {
        let timeout;
        return function executedFunction(...args) {
            const later = () => {
                clearTimeout(timeout);
                func(...args);
            };
            clearTimeout(timeout);
            timeout = setTimeout(later, wait);
        };
    }
    
    /**
     * Throttle function
     * @param {function} func - Function to throttle
     * @param {number} limit - Time limit in milliseconds
     * @returns {function} Throttled function
     */
    throttle(func, limit) {
        let inThrottle;
        return function() {
            const args = arguments;
            const context = this;
            if (!inThrottle) {
                func.apply(context, args);
                inThrottle = true;
                setTimeout(() => inThrottle = false, limit);
            }
        };
    }
}

// Create global instance
window.vfUI = new VaporFrameUI();

// Export for module systems
if (typeof module !== 'undefined' && module.exports) {
    module.exports = VaporFrameUI;
} 