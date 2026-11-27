// Valida a URL da imagem antes de carregar
function validateImageUrl(value) {
  if (typeof value !== 'string' || value.length > 2048) {
    return null;
  }

  try {
    const url = new URL(value);

    if (url.protocol !== 'https:' || !url.hostname || url.username || url.password) {
      return null;
    }

    return url;
  } catch {
    return null;
  }
}

// Cria a imagem com fallback para o nome da receita
function createRecipeImage(recipe) {
  const image = document.createElement('div');
  image.className = 'recipe-image';
  image.textContent = recipe.name;

  const imageUrl = validateImageUrl(recipe.image_url);

  if (!imageUrl) {
    return image;
  }

  const photo = document.createElement('img');
  photo.className = 'recipe-card-photo';
  photo.alt = '';
  photo.loading = 'lazy';
  photo.referrerPolicy = 'no-referrer';

  photo.addEventListener('load', () => {
    photo.classList.add('is-loaded');
  });

  photo.addEventListener('error', () => photo.remove());

  image.append(photo);

  photo.src = `https://${imageUrl.host}${imageUrl.pathname}${imageUrl.search}${imageUrl.hash}`;

  return image;
}

// Mostra as porcoes e os tempos da receita
function createRecipeTimes(recipe) {
  const details = document.createElement('span');
  details.className = 'recipe-time';

  const parts = [];

  if (recipe.servings != null) {
    parts.push(`${recipe.servings} servings`);
  }

  if (recipe.prep_minutes != null) {
    parts.push(`Prep: ${recipe.prep_minutes} min`);
  }

  if (recipe.cook_minutes != null) {
    parts.push(`Cook: ${recipe.cook_minutes} min`);
  }

  details.textContent = parts.join(' · ');

  return details;
}

// Funcao compartilhada para criar os cartoes
function createSharedRecipeCard(
  recipe,
  { community = false, showDetails = false, nested = false } = {}
) {
  const card = document.createElement('a');

  card.className = nested ? 'recipe-card-link' : 'recipe-card recipe-card-link';

  card.href = `recipe.html?id=${encodeURIComponent(recipe.id)}`;

  if (community) {
    card.href += '&source=discover';
  }

  const content = document.createElement('div');
  content.className = 'recipe-content';

  const title = document.createElement('h3');
  title.textContent = recipe.name;
  content.append(title);

  if (community) {
    const author = document.createElement('p');
    author.className = 'recipe-author';
    author.textContent = `Shared by ${recipe.author}`;
    content.append(author);
  }

  const ingredients = document.createElement('p');
  const ingredientText = recipe.ingredient_list || '';

  ingredients.textContent =
    ingredientText.length > 140 ? ingredientText.slice(0, 140) + '…' : ingredientText;

  content.append(ingredients);

  if (showDetails) {
    content.append(createRecipeTimes(recipe));
  }

  card.append(createRecipeImage(recipe), content);

  return card;
}
