(function () {
	'use strict';

	function isLocalStorageAvailable() {
		try {
			const testKey = '__localStorage_test__';
			localStorage.setItem(testKey, 'test');
			const retrieved = localStorage.getItem(testKey);
			localStorage.removeItem(testKey);
			return retrieved === 'test';
		} catch (e) {
			return false;
		}
	}

	function hasAnalyticsConsent() {
		if (!isLocalStorageAvailable()) {
			return false;
		}
		try {
			return localStorage.getItem('analyticsConsent') === 'true';
		} catch (e) {
			console.warn('Failed to read analytics consent:', e);
			return false;
		}
	}

	function updateConsent(granted) {
		const tryUpdate = () => {
			if (typeof window.gtag === 'function') {
				window.gtag('consent', 'update', {
					'analytics_storage': granted ? 'granted' : 'denied'
				});
				console.log('Analytics consent updated:', granted ? 'granted' : 'denied');
			} else {
				setTimeout(tryUpdate, 100);
			}
		};
		tryUpdate();
	}

	window.setAnalyticsConsent = function (consent) {
		if (!isLocalStorageAvailable()) {
			console.warn('localStorage is not available. Analytics consent cannot be saved.');
			updateConsent(consent);
			return;
		}
		try {
			localStorage.setItem('analyticsConsent', consent ? 'true' : 'false');
			updateConsent(consent);
			if (consent) {
				console.log('Google Analytics consent granted');
			} else {
				console.log('Google Analytics consent denied');
			}
		} catch (e) {
			console.warn('Failed to set analytics consent:', e);
		}
	};

	function isInstalledPWA() {
		if (window.navigator.standalone === true) {
			return true;
		}
		if (window.matchMedia('(display-mode: standalone)').matches) {
			return true;
		}
		if (window.matchMedia('(display-mode: fullscreen)').matches) {
			return true;
		}
		if (window.matchMedia('(display-mode: minimal-ui)').matches) {
			return true;
		}
		return false;
	}

	function initializeGA() {
		if (window.__gaInitialized) {
			return;
		}

		try {
			window.dataLayer = window.dataLayer || [];
			function gtag() {
				dataLayer.push(arguments);
			}
			window.gtag = gtag;
			
			gtag('consent', 'default', {
				'analytics_storage': 'denied',
				'ad_storage': 'denied',
				'ad_user_data': 'denied',
				'ad_personalization': 'denied'
			});

			const script = document.createElement('script');
			script.async = true;
			script.src = 'https://www.googletagmanager.com/gtag/js?id=G-MXJH30352W';
			document.head.appendChild(script);

			gtag('js', new Date());
			gtag('config', 'G-MXJH30352W');

			window.__gaInitialized = true;
			console.log('Google Analytics initialized with Consent Mode v2 (default: denied)');
		} catch (e) {
			console.warn('Failed to initialize Google Analytics:', e);
		}
	}

	if (isInstalledPWA()) {
		console.log('Google Analytics is disabled (running as installed PWA)');
		return;
	}

	const IS_PRODUCTION = window.location.hostname === 'or18.github.io' || true;

	if (!IS_PRODUCTION) {
		console.log('Google Analytics is disabled (not production environment)');
		return;
	}

	initializeGA();

	if (hasAnalyticsConsent()) {
		updateConsent(true);
	} else if (isLocalStorageAvailable()) {
		const consentValue = localStorage.getItem('analyticsConsent');
		if (consentValue === null) {
			console.log('Google Analytics waiting for consent');
			showConsentBanner();
		} else {
			console.log('Google Analytics consent previously denied');
			updateConsent(false);
		}
	} else {
		console.log('Google Analytics running with Consent Mode (localStorage unavailable)');
	}

	function showConsentBanner() {
		if (!isLocalStorageAvailable()) {
			console.log('Google Analytics consent banner not shown (localStorage unavailable)');
			return;
		}

		if (document.getElementById('ga-consent-banner')) {
			return;
		}

		const banner = document.createElement('div');
		banner.id = 'ga-consent-banner';
		banner.style.cssText = `
            position: fixed;
            bottom: 20px;
            left: 50%;
            transform: translateX(-50%);
            background: #1e1e1e;
            color: #ffffff;
            padding: 12px 20px;
            border-radius: 8px;
            border: 1px solid #41417f;
            box-shadow: 0 4px 12px rgba(0,0,0,0.3);
            z-index: 10000;
            font-family: Arial, sans-serif;
            font-size: 14px;
            max-width: 90%;
            width: 500px;
            display: flex;
            align-items: center;
            gap: 12px;
            animation: slideUp 0.3s ease-out;
        `;

		const message = document.createElement('span');
		message.style.flex = '1';
		message.innerHTML = 'We use analytics to improve your experience. <a href="https://policies.google.com/technologies/partner-sites" target="_blank" rel="noopener noreferrer" style="color: #8888ff; text-decoration: underline; font-size: 14px;">Learn more</a>';

		const buttonContainer = document.createElement('div');
		buttonContainer.style.cssText = 'display: flex; gap: 8px;';

		const declineBtn = document.createElement('button');
		declineBtn.textContent = 'Decline';
		declineBtn.style.cssText = `
            background: transparent;
            color: #aaaaaa;
            border: 1px solid #555555;
            padding: 8px 16px;
            border-radius: 4px;
            cursor: pointer;
            font-size: 14px;
            white-space: nowrap;
        `;
		declineBtn.onmouseover = () => {
			declineBtn.style.background = '#2a2a2a';
			declineBtn.style.color = '#ffffff';
		};
		declineBtn.onmouseout = () => {
			declineBtn.style.background = 'transparent';
			declineBtn.style.color = '#aaaaaa';
		};
		declineBtn.onclick = () => {
			clearTimeout(autoHideTimer);
			window.setAnalyticsConsent(false);
			banner.style.animation = 'slideDown 0.3s ease-in';
			setTimeout(() => banner.remove(), 300);
		};

		const acceptBtn = document.createElement('button');
		acceptBtn.textContent = 'Accept';
		acceptBtn.style.cssText = `
            background: #56567c;
            color: #ffffff;
            border: none;
            padding: 8px 20px;
            border-radius: 4px;
            cursor: pointer;
            font-size: 14px;
            white-space: nowrap;
        `;
		acceptBtn.onmouseover = () => acceptBtn.style.background = '#4a4a6d';
		acceptBtn.onmouseout = () => acceptBtn.style.background = '#56567c';
		acceptBtn.onclick = () => {
			clearTimeout(autoHideTimer);
			window.setAnalyticsConsent(true);
			banner.style.animation = 'slideDown 0.3s ease-in';
			setTimeout(() => banner.remove(), 300);
		};

		buttonContainer.appendChild(declineBtn);
		buttonContainer.appendChild(acceptBtn);

		banner.appendChild(message);
		banner.appendChild(buttonContainer);

		if (!document.getElementById('ga-consent-styles')) {
			const style = document.createElement('style');
			style.id = 'ga-consent-styles';
			style.textContent = `
                @keyframes slideUp {
                    from {
                        transform: translateX(-50%) translateY(100px);
                        opacity: 0;
                    }
                    to {
                        transform: translateX(-50%) translateY(0);
                        opacity: 1;
                    }
                }
                @keyframes slideDown {
                    from {
                        transform: translateX(-50%) translateY(0);
                        opacity: 1;
                    }
                    to {
                        transform: translateX(-50%) translateY(100px);
                        opacity: 0;
                    }
                }
            `;
			document.head.appendChild(style);
		}

		if (document.body) {
			document.body.appendChild(banner);
		} else {
			document.addEventListener('DOMContentLoaded', () => {
				document.body.appendChild(banner);
			});
		}

		const autoHideTimer = setTimeout(() => {
			console.log('Google Analytics consent banner timed out (no consent given)');
			window.setAnalyticsConsent(false);
			banner.style.animation = 'slideDown 0.3s ease-in';
			setTimeout(() => banner.remove(), 300);
		}, 10000);
	}
})();