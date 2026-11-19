const plansMessage = document.querySelector('#plans-message');
const freeStatus = document.querySelector('#free-status');
const premiumStatus = document.querySelector('#premium-status');
const upgradeButton = document.querySelector('#upgrade-button');
const upgradeDialog = document.querySelector('#upgrade-dialog');
const upgradeConfirm = document.querySelector('#upgrade-confirm');
const upgradeCancel = document.querySelector('#upgrade-cancel');
const upgradeError = document.querySelector('#upgrade-error');

// Marks the user's plan and offers the upgrade to Free users only
function showCurrentPlan(plan) {
  freeStatus.hidden = plan !== 'Free';
  premiumStatus.hidden = plan !== 'Premium';
  upgradeButton.hidden = plan !== 'Free';
}

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
    const currentPlan = data.user?.tier;

    showCurrentPlan(currentPlan);

    if (currentPlan === 'Free' || currentPlan === 'Premium') {
      plansMessage.textContent = 'You are currently on the ' + currentPlan + ' plan.';
    } else {
      plansMessage.textContent = 'You are signed in, but your current plan could not be read.';
    }
  } catch (error) {
    plansMessage.textContent =
      'Plan comparison is available, but account information could not be loaded.';
  }
}

// Sends the upgrade after the user confirms it in the dialog
async function upgradeToPremium() {
  upgradeError.hidden = true;
  upgradeConfirm.disabled = true;

  try {
    const response = await fetch('/api/account/upgrade', {
      method: 'POST',
      credentials: 'same-origin',
    });
    const body = await response.json().catch(() => ({}));

    if (response.status === 401) {
      window.location.assign('index.html');
      return;
    }

    // 409 means the account is already Premium, for example after upgrading in another tab
    if (!response.ok && response.status !== 409) {
      upgradeError.textContent = body.error || 'Unable to upgrade your plan. Please try again.';
      upgradeError.hidden = false;
      return;
    }

    upgradeDialog.close();
    showCurrentPlan('Premium');
    plansMessage.textContent = response.ok
      ? 'You are now on the Premium plan and can save up to 500 recipes.'
      : 'You are already on the Premium plan.';
  } catch (error) {
    upgradeError.textContent = 'Could not connect to the server. Please try again.';
    upgradeError.hidden = false;
  } finally {
    upgradeConfirm.disabled = false;
  }
}

upgradeButton.addEventListener('click', () => {
  upgradeError.hidden = true;
  upgradeDialog.showModal();
});

upgradeCancel.addEventListener('click', () => upgradeDialog.close());
upgradeConfirm.addEventListener('click', () => void upgradeToPremium());

void loadCurrentPlan();
