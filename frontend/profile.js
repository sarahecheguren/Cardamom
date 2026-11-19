const profileMessage = document.querySelector('#profile-message');
const profileName = document.querySelector('#profile-name');
const profileEmail = document.querySelector('#profile-email');
const profilePlan = document.querySelector('#profile-plan');
const profileRecipeCount = document.querySelector('#profile-recipe-count');
const profileUsageText = document.querySelector('#profile-usage-text');
const profileUsageBar = document.querySelector('#profile-usage-bar');
const profileUpgrade = document.querySelector('#profile-upgrade');

async function loadProfile() {
  profileMessage.textContent = 'Checking your account...';

  try {
    const response = await fetch('/api/auth/me', {
      credentials: 'same-origin',
      cache: 'no-store',
    });

    if (response.status === 401) {
      window.location.replace('index.html');
      return;
    }

    if (!response.ok) {
      profileMessage.textContent = 'Unable to check your account. Please try again.';
      return;
    }

    const data = await response.json();
    const user = data.user || {};

    if (typeof user.display_name === 'string' && user.display_name) {
      profileName.textContent = user.display_name;
    }

    if (typeof user.email === 'string' && user.email) {
      profileEmail.textContent = user.email;
    }

    if (typeof user.tier === 'string' && user.tier) {
      profilePlan.textContent = user.tier;
    }

    // Only Free users are offered the upgrade.
    profileUpgrade.hidden = user.tier !== 'Free';

    const count = user.recipe_count;
    const limit = user.recipe_limit;

    if (Number.isInteger(count)) {
      profileRecipeCount.textContent = count + (count === 1 ? ' recipe saved' : ' recipes saved');
    }

    if (Number.isInteger(count) && Number.isInteger(limit) && limit > 0) {
      profileUsageText.textContent = count + ' / ' + limit + ' recipes';
      profileUsageBar.max = limit;
      profileUsageBar.value = Math.min(count, limit);
    }

    profileMessage.textContent = '';
  } catch (error) {
    profileMessage.textContent = 'Could not connect to the server. Please try again.';
  }
}

void loadProfile();
