const plansMessage = document.querySelector('#plans-message');
const freeStatus = document.querySelector('#free-status');
const premiumStatus = document.querySelector('#premium-status');

// Checks the user's current subscription plan
async function loadCurrentPlan() {
  // Allow the page to be previewed without running the backend
  if (window.location.protocol === 'file:') {
    plansMessage.textContent =
      'Plan comparison preview. Account information requires the Cardamom server.';
    return;
  }

  try {
    const response = await fetch('/api/auth/me', {
      credentials: 'same-origin',
      cache: 'no-store',
    });

    // Visitors can compare plans without logging in
    if (response.status === 401) {
      plansMessage.textContent = 'Compare our available plans. Log in to see your current plan.';
      return;
    }

    if (!response.ok) {
      plansMessage.textContent = 'Unable to check your current plan right now.';
      return;
    }

    const data = await response.json();
    const currentPlan = data.user?.tier_name;

    if (currentPlan === 'Free') {
      freeStatus.hidden = false;
      plansMessage.textContent = 'You are currently on the Free plan.';
    } else if (currentPlan === 'Premium') {
      premiumStatus.hidden = false;
      plansMessage.textContent = 'You are currently on the Premium plan.';
    } else {
      plansMessage.textContent =
        'You are signed in. Your current plan will appear after backend integration.';
    }
  } catch (error) {
    plansMessage.textContent =
      'Plan comparison is available, but account information could not be loaded.';
  }
}

void loadCurrentPlan();
