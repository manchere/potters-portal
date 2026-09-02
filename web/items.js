const banner = document.getElementById("banner");
const itemsBody = document.getElementById("items-body");
const searchInput = document.getElementById("search-input");

const overlay = document.getElementById("edit-overlay");
const editName = document.getElementById("edit-name");
const editDescription = document.getElementById("edit-description");
const editQuantity = document.getElementById("edit-quantity");
const editLocation = document.getElementById("edit-location");
const editStatus = document.getElementById("edit-status");
const editCategory = document.getElementById("edit-category");
const editTagList = document.getElementById("edit-tag-list");
const editImageInput = document.getElementById("edit-image-input");
const editImagePreview = document.getElementById("edit-image-preview");

const deleteOverlay = document.getElementById("delete-overlay");
const deleteImagePreview = document.getElementById("delete-image-preview");
const deleteImagePlaceholder = document.getElementById("delete-image-placeholder");
const deleteItemName = document.getElementById("delete-item-name");

let tags = [];
let categories = [];
let allItems = [];
let editingItemId = null;
let editSelectedImageDataUrl = null;
let pendingDeleteItem = null;

function showBanner(message, kind) {
  banner.textContent = message;
  banner.className = `banner ${kind}`;
  banner.hidden = false;
}

async function api(path, options) {
  const response = await fetch(`/api${path}`, {
    headers: { "Content-Type": "application/json" },
    ...options,
  });
  const body = await response.json().catch(() => ({}));
  if (!response.ok) {
    throw new Error(body.error || `request failed (${response.status})`);
  }
  return body;
}

// Same downscale-before-upload approach as app.js, duplicated since there's
// no build step / shared module setup between pages.
function resizeImageToDataUrl(file, maxDimension = 1280, quality = 0.82) {
  return new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.onerror = () => reject(new Error("Could not read the image file."));
    reader.onload = () => {
      const img = new Image();
      img.onerror = () => reject(new Error("Could not decode the image file."));
      img.onload = () => {
        const scale = Math.min(1, maxDimension / Math.max(img.width, img.height));
        const canvas = document.createElement("canvas");
        canvas.width = Math.round(img.width * scale);
        canvas.height = Math.round(img.height * scale);
        const ctx = canvas.getContext("2d");
        ctx.drawImage(img, 0, 0, canvas.width, canvas.height);
        resolve(canvas.toDataURL("image/jpeg", quality));
      };
      img.src = reader.result;
    };
    reader.readAsDataURL(file);
  });
}

function categoryName(id) {
  const category = categories.find((c) => c.id === id);
  return category ? category.name : "None";
}

function renderTagPills(ids) {
  const cell = document.createElement("td");
  if (ids.length === 0) {
    cell.textContent = "—";
    return cell;
  }
  for (const id of ids) {
    const tag = tags.find((t) => t.id === id);
    const pill = document.createElement("span");
    pill.className = "tag-pill";
    pill.textContent = tag ? tag.name : `#${id}`;
    pill.style.background = tag ? tag.color : "#9aa1ac";
    cell.appendChild(pill);
  }
  return cell;
}

function renderThumbCell(item) {
  const cell = document.createElement("td");
  if (item.image_url) {
    const img = document.createElement("img");
    img.className = "item-thumb";
    img.src = item.image_url;
    img.alt = item.name;
    cell.appendChild(img);
  } else {
    const placeholder = document.createElement("div");
    placeholder.className = "item-thumb-placeholder";
    placeholder.textContent = "No image";
    cell.appendChild(placeholder);
  }
  return cell;
}

function capitalize(s) {
  return s.charAt(0).toUpperCase() + s.slice(1);
}

async function loadReferenceData() {
  [categories, tags] = await Promise.all([api("/categories"), api("/tags")]);
}

async function loadItems() {
  allItems = await api("/items");
  renderItems();
}

function renderItems() {
  const query = searchInput.value.trim().toLowerCase();
  const items = query ? allItems.filter((item) => item.name.toLowerCase().includes(query)) : allItems;

  itemsBody.innerHTML = "";
  for (const item of items) {
    const tr = document.createElement("tr");

    const statusCell = document.createElement("td");
    statusCell.textContent = capitalize(item.status);
    statusCell.className = `status status-${item.status}`;

    const actionsCell = document.createElement("td");
    const editBtn = document.createElement("button");
    editBtn.type = "button";
    editBtn.className = "btn btn-secondary btn-small";
    editBtn.textContent = "Edit";
    editBtn.addEventListener("click", () => openEditModal(item));
    const deleteBtn = document.createElement("button");
    deleteBtn.type = "button";
    deleteBtn.className = "btn btn-danger btn-small";
    deleteBtn.textContent = "Delete";
    deleteBtn.addEventListener("click", () => openDeleteModal(item));
    actionsCell.appendChild(editBtn);
    actionsCell.appendChild(deleteBtn);

    const nameCell = document.createElement("td");
    nameCell.textContent = item.name;
    const quantityCell = document.createElement("td");
    quantityCell.textContent = item.quantity;
    const locationCell = document.createElement("td");
    locationCell.textContent = item.location;
    const categoryCell = document.createElement("td");
    categoryCell.textContent = categoryName(item.category_id);

    tr.appendChild(renderThumbCell(item));
    tr.appendChild(nameCell);
    tr.appendChild(quantityCell);
    tr.appendChild(locationCell);
    tr.appendChild(statusCell);
    tr.appendChild(categoryCell);
    tr.appendChild(renderTagPills(item.tag_ids));
    tr.appendChild(actionsCell);
    itemsBody.appendChild(tr);
  }

  if (items.length === 0) {
    const tr = document.createElement("tr");
    const message = query ? `No items match "${searchInput.value.trim()}".` : `No items yet. <a href="/">Add one</a>.`;
    tr.innerHTML = `<td colspan="8" class="empty-row">${message}</td>`;
    itemsBody.appendChild(tr);
  }
}

searchInput.addEventListener("input", renderItems);

function openDeleteModal(item) {
  pendingDeleteItem = item;
  deleteItemName.textContent = item.name;
  if (item.image_url) {
    deleteImagePreview.src = item.image_url;
    deleteImagePreview.hidden = false;
    deleteImagePlaceholder.hidden = true;
  } else {
    deleteImagePreview.hidden = true;
    deleteImagePlaceholder.hidden = false;
  }
  deleteOverlay.hidden = false;
}

function closeDeleteModal() {
  deleteOverlay.hidden = true;
  pendingDeleteItem = null;
}

document.getElementById("delete-cancel").addEventListener("click", closeDeleteModal);
deleteOverlay.addEventListener("click", (event) => {
  if (event.target === deleteOverlay) closeDeleteModal();
});

document.getElementById("delete-confirm").addEventListener("click", async () => {
  if (!pendingDeleteItem) return;
  try {
    await api(`/items/${pendingDeleteItem.id}`, { method: "DELETE" });
    closeDeleteModal();
    await loadItems();
  } catch (err) {
    showBanner(err.message, "error");
  }
});

function renderEditCategoryOptions() {
  editCategory.innerHTML = '<option value="">None</option>';
  for (const category of categories) {
    const option = document.createElement("option");
    option.value = category.id;
    option.textContent = category.name;
    editCategory.appendChild(option);
  }
}

function renderEditTagList(checkedIds) {
  editTagList.innerHTML = "";
  if (tags.length === 0) {
    const li = document.createElement("li");
    li.className = "empty";
    li.textContent = "No tags yet.";
    editTagList.appendChild(li);
    return;
  }
  for (const tag of tags) {
    const li = document.createElement("li");
    const checkbox = document.createElement("input");
    checkbox.type = "checkbox";
    checkbox.value = tag.id;
    checkbox.id = `edit-tag-${tag.id}`;
    checkbox.checked = checkedIds.includes(tag.id);
    const swatch = document.createElement("span");
    swatch.className = "tag-swatch";
    swatch.style.background = tag.color;
    const label = document.createElement("label");
    label.htmlFor = checkbox.id;
    label.textContent = tag.name;
    li.appendChild(checkbox);
    li.appendChild(swatch);
    li.appendChild(label);
    editTagList.appendChild(li);
  }
}

function openEditModal(item) {
  editingItemId = item.id;
  editSelectedImageDataUrl = null;
  editImageInput.value = "";
  editName.value = item.name;
  editDescription.value = item.description;
  editQuantity.value = item.quantity;
  editLocation.value = item.location;
  editStatus.value = item.status;
  renderEditCategoryOptions();
  editCategory.value = item.category_id ?? "";
  renderEditTagList(item.tag_ids);

  if (item.image_url) {
    editImagePreview.src = item.image_url;
    editImagePreview.hidden = false;
  } else {
    editImagePreview.hidden = true;
    editImagePreview.src = "";
  }

  overlay.hidden = false;
}

function closeEditModal() {
  overlay.hidden = true;
  editingItemId = null;
}

editImageInput.addEventListener("change", async () => {
  const file = editImageInput.files[0];
  if (!file) return;
  try {
    editSelectedImageDataUrl = await resizeImageToDataUrl(file);
    editImagePreview.src = editSelectedImageDataUrl;
    editImagePreview.hidden = false;
  } catch (err) {
    showBanner(err.message, "error");
  }
});

document.getElementById("edit-cancel").addEventListener("click", closeEditModal);
overlay.addEventListener("click", (event) => {
  if (event.target === overlay) closeEditModal();
});

document.getElementById("edit-save").addEventListener("click", async () => {
  const name = editName.value.trim();
  if (!name) {
    showBanner("Name is required.", "error");
    return;
  }

  const tagIds = Array.from(editTagList.querySelectorAll("input[type=checkbox]:checked")).map((el) => Number(el.value));

  const payload = {
    name,
    description: editDescription.value.trim(),
    quantity: Number(editQuantity.value) || 0,
    location: editLocation.value.trim(),
    status: editStatus.value,
    category_id: editCategory.value ? Number(editCategory.value) : null,
    tag_ids: tagIds,
  };

  try {
    await api(`/items/${editingItemId}`, { method: "PUT", body: JSON.stringify(payload) });
    if (editSelectedImageDataUrl) {
      await api(`/items/${editingItemId}/image`, {
        method: "POST",
        body: JSON.stringify({ image_base64: editSelectedImageDataUrl }),
      });
    }
    closeEditModal();
    await loadItems();
  } catch (err) {
    showBanner(err.message, "error");
  }
});

async function init() {
  try {
    await loadReferenceData();
    await loadItems();
  } catch (err) {
    showBanner(err.message, "error");
  }
}

init();
