/* ============================================================
   PayrollPro — script.js
   Same payroll logic as the C version, rewritten in JS.
   Data persisted via localStorage.
   ============================================================ */

'use strict';

/* ─── Constants ─────────────────────────────────────────────── */
const OT_RATE    = 100;       // Rs. per hour
const STORAGE_KEY = 'payrollpro_v1';
const MAX_EMP    = 100;

/* ─── State ─────────────────────────────────────────────────── */
let employees = [];           // Array of employee objects
let pendingDeleteId = null;   // ID waiting for confirm-delete

/* ─── Init ──────────────────────────────────────────────────── */
document.addEventListener('DOMContentLoaded', () => {
  loadFromStorage();
  setupNavigation();
  setupFormListeners();
  refreshAll();
});

/* ════════════════════════════════════════════════════════════
   PAYROLL LOGIC  (mirroring the C implementation)
   ════════════════════════════════════════════════════════════ */

/**
 * getTaxRate — returns decimal tax rate based on gross pay slab.
 * Mirrors C's getTaxRate().
 */
function getTaxRate(gross) {
  if (gross <= 30000) return 0.05;
  if (gross <= 60000) return 0.10;
  return 0.15;
}

/**
 * calculatePayroll — fills computed payroll fields on an employee object.
 * Mirrors C's calculatePayroll().
 */
function calculatePayroll(emp) {
  emp.otPay     = emp.otHours * OT_RATE;
  emp.grossPay  = emp.basicPay + emp.otPay;
  emp.taxRate   = getTaxRate(emp.grossPay);
  emp.taxAmount = emp.grossPay * emp.taxRate;
  emp.netPay    = emp.grossPay - emp.taxAmount;
  return emp;
}

/* ════════════════════════════════════════════════════════════
   NAVIGATION
   ════════════════════════════════════════════════════════════ */
function setupNavigation() {
  document.querySelectorAll('.nav-item').forEach(item => {
    item.addEventListener('click', () => navigate(item.dataset.section));
  });
}

function navigate(sectionId) {
  /* Update nav */
  document.querySelectorAll('.nav-item').forEach(n => {
    n.classList.toggle('active', n.dataset.section === sectionId);
  });
  /* Show section */
  document.querySelectorAll('.section').forEach(s => {
    s.classList.toggle('active', s.id === `section-${sectionId}`);
  });
  /* Refresh if switching to employees */
  if (sectionId === 'employees') renderTable();
  if (sectionId === 'dashboard') renderDashboard();
}

/* ════════════════════════════════════════════════════════════
   ADD EMPLOYEE
   ════════════════════════════════════════════════════════════ */
function setupFormListeners() {
  /* Live payroll preview as user types */
  ['fBasic', 'fOT'].forEach(id => {
    document.getElementById(id).addEventListener('input', updatePreview);
  });
  /* ID uniqueness hint */
  document.getElementById('fEmpId').addEventListener('input', checkIdHint);
}

function checkIdHint() {
  const raw  = document.getElementById('fEmpId').value.trim().toUpperCase();
  const hint = document.getElementById('idHint');
  if (!raw) { hint.textContent = ''; hint.className = 'form-hint'; return; }
  if (isDuplicateId(raw)) {
    hint.textContent = `⚠ "${raw}" already exists`;
    hint.className = 'form-hint error';
  } else {
    hint.textContent = `✔ "${raw}" is available`;
    hint.className = 'form-hint';
    hint.style.color = 'var(--green)';
  }
}

function updatePreview() {
  const basic = parseFloat(document.getElementById('fBasic').value) || 0;
  const ot    = parseFloat(document.getElementById('fOT').value)    || 0;
  const dummy = calculatePayroll({ basicPay: basic, otHours: ot });

  document.getElementById('pvOT').textContent    = `Rs. ${fmt(dummy.otPay)}`;
  document.getElementById('pvGross').textContent = `Rs. ${fmt(dummy.grossPay)}`;
  document.getElementById('pvTax').textContent   =
    `Rs. ${fmt(dummy.taxAmount)} (${(dummy.taxRate*100).toFixed(0)}%)`;
  document.getElementById('pvNet').textContent   = `Rs. ${fmt(dummy.netPay)}`;
}

function addEmployee() {
  clearFormMsg();

  const empId    = document.getElementById('fEmpId').value.trim().toUpperCase();
  const name     = document.getElementById('fName').value.trim();
  const dept     = document.getElementById('fDept').value.trim();
  const basicPay = parseFloat(document.getElementById('fBasic').value);
  const otHours  = parseFloat(document.getElementById('fOT').value) || 0;

  /* Validation */
  if (!empId)          return showFormMsg('Employee ID is required.', 'error');
  if (!name)           return showFormMsg('Full Name is required.', 'error');
  if (!dept)           return showFormMsg('Department is required.', 'error');
  if (!basicPay || basicPay < 1)
                       return showFormMsg('Basic Pay must be ≥ Rs. 1.', 'error');
  if (otHours < 0)     return showFormMsg('OT Hours cannot be negative.', 'error');
  if (employees.length >= MAX_EMP)
                       return showFormMsg(`Max ${MAX_EMP} employees reached.`, 'error');
  if (isDuplicateId(empId))
                       return showFormMsg(`ID "${empId}" already exists. Use a unique ID.`, 'error');

  const emp = calculatePayroll({ empId, name, dept, basicPay, otHours });
  employees.push(emp);
  saveToStorage();
  refreshAll();

  showFormMsg(`✔ ${name} added! Net Pay: Rs. ${fmt(emp.netPay)}`, 'success');
  clearForm(false);      // clear inputs but keep success message
  showToast(`${name} added successfully`, 'success');
}

function clearForm(clearMsg = true) {
  ['fEmpId','fName','fDept','fBasic','fOT'].forEach(id => {
    document.getElementById(id).value = '';
    document.getElementById(id).classList.remove('error');
  });
  document.getElementById('idHint').textContent = '';
  updatePreview();
  if (clearMsg) clearFormMsg();
}

function showFormMsg(msg, type) {
  const el = document.getElementById('formMsg');
  el.textContent = msg;
  el.className = `form-msg ${type}`;
}

function clearFormMsg() {
  const el = document.getElementById('formMsg');
  el.textContent = '';
  el.className = 'form-msg';
}

/* ════════════════════════════════════════════════════════════
   TABLE — DISPLAY ALL
   ════════════════════════════════════════════════════════════ */
function renderTable() {
  const query  = (document.getElementById('searchInput')?.value || '').toLowerCase();
  const tbody  = document.getElementById('empTbody');
  const filtered = employees.filter(e =>
    e.empId.toLowerCase().includes(query) ||
    e.name.toLowerCase().includes(query)
  );

  if (filtered.length === 0) {
    tbody.innerHTML = `<tr><td colspan="9" class="empty-cell">
      ${employees.length === 0 ? 'No employees yet.' : 'No results match your search.'}
    </td></tr>`;
    return;
  }

  tbody.innerHTML = filtered.map((e, i) => `
    <tr>
      <td><span class="td-id">${esc(e.empId)}</span></td>
      <td class="td-name">${esc(e.name)}</td>
      <td><span class="td-dept">${esc(e.dept)}</span></td>
      <td class="td-num">Rs. ${fmt(e.basicPay)}</td>
      <td class="td-num">${e.otHours.toFixed(1)} h</td>
      <td class="td-num">Rs. ${fmt(e.grossPay)}</td>
      <td class="td-num td-tax">${(e.taxRate*100).toFixed(0)}%</td>
      <td class="td-num td-net">Rs. ${fmt(e.netPay)}</td>
      <td>
        <div class="actions-cell">
          <button class="btn-icon btn-icon-slip"
            onclick="openPayslip('${esc(e.empId)}')">Payslip</button>
          <button class="btn-icon btn-icon-del"
            onclick="confirmDelete('${esc(e.empId)}')">Delete</button>
        </div>
      </td>
    </tr>
  `).join('');
}

/* ════════════════════════════════════════════════════════════
   DASHBOARD
   ════════════════════════════════════════════════════════════ */
function renderDashboard() {
  const count    = employees.length;
  const totalNet = employees.reduce((s, e) => s + e.netPay, 0);
  const totalTax = employees.reduce((s, e) => s + e.taxAmount, 0);
  const avgNet   = count ? totalNet / count : 0;

  document.getElementById('statCount').textContent = count;
  document.getElementById('statNet').textContent   = `Rs. ${fmtK(totalNet)}`;
  document.getElementById('statAvg').textContent   = `Rs. ${fmtK(avgNet)}`;
  document.getElementById('statTax').textContent   = `Rs. ${fmtK(totalTax)}`;
  document.getElementById('storageBadge').textContent = `${count} record${count===1?'':'s'} stored`;

  /* bar fill (percentage of max capacity) */
  const pct = Math.min((count / MAX_EMP) * 100, 100);
  document.getElementById('barCount').style.width = `${pct}%`;

  /* Recent employees list — last 5 */
  const recentList = document.getElementById('recentList');
  if (count === 0) {
    recentList.innerHTML = '<div class="empty-state">No employees added yet.</div>';
    return;
  }
  const recent = [...employees].slice(-5).reverse();
  recentList.innerHTML = recent.map(e => `
    <div class="recent-row">
      <div>
        <div class="recent-name">${esc(e.name)}</div>
        <div class="recent-id">${esc(e.empId)} · ${esc(e.dept)}</div>
      </div>
      <div class="recent-net">Rs. ${fmt(e.netPay)}</div>
    </div>
  `).join('');
}

/* ════════════════════════════════════════════════════════════
   PAYSLIP MODAL
   ════════════════════════════════════════════════════════════ */
function openPayslip(empId) {
  const e = employees.find(x => x.empId === empId);
  if (!e) return showToast('Employee not found.', 'error');

  document.getElementById('slipId').textContent    = e.empId;
  document.getElementById('slipName').textContent  = e.name;
  document.getElementById('slipDept').textContent  = e.dept;
  document.getElementById('slipBasic').textContent = `Rs. ${fmt(e.basicPay)}`;
  document.getElementById('slipOTLabel').textContent =
    `Overtime (${e.otHours.toFixed(1)}h × Rs.${OT_RATE}/h)`;
  document.getElementById('slipOTPay').textContent  = `Rs. ${fmt(e.otPay)}`;
  document.getElementById('slipGross').textContent  = `Rs. ${fmt(e.grossPay)}`;
  document.getElementById('slipTaxLabel').textContent =
    `Income Tax (${(e.taxRate*100).toFixed(0)}%)`;
  document.getElementById('slipTaxAmt').textContent   = `Rs. ${fmt(e.taxAmount)}`;
  document.getElementById('slipTotalDed').textContent = `Rs. ${fmt(e.taxAmount)}`;
  document.getElementById('slipNet').textContent      = `Rs. ${fmt(e.netPay)}`;

  openModal('payslipModal', 'modalBackdrop');
}

function closeModal() {
  closeModalById('payslipModal', 'modalBackdrop');
}

function printPayslip() {
  window.print();
}

/* ════════════════════════════════════════════════════════════
   DELETE
   ════════════════════════════════════════════════════════════ */
function confirmDelete(empId) {
  const e = employees.find(x => x.empId === empId);
  if (!e) return;
  pendingDeleteId = empId;
  document.getElementById('confirmMsg').innerHTML =
    `Delete <strong>${esc(e.name)}</strong> (${esc(e.empId)})?<br>
     <span style="color:var(--text-muted);font-size:.82rem">This cannot be undone.</span>`;
  document.getElementById('confirmYes').onclick = executeDelete;
  openModal('confirmModal', 'confirmBackdrop');
}

function executeDelete() {
  if (!pendingDeleteId) return;
  const name = employees.find(e => e.empId === pendingDeleteId)?.name || '';
  employees = employees.filter(e => e.empId !== pendingDeleteId);
  pendingDeleteId = null;
  saveToStorage();
  refreshAll();
  closeConfirm();
  showToast(`${name} deleted`, 'error');
}

function closeConfirm() {
  closeModalById('confirmModal', 'confirmBackdrop');
}

function clearAllConfirm() {
  if (employees.length === 0) return showToast('No records to clear.', 'error');
  pendingDeleteId = '__ALL__';
  document.getElementById('confirmMsg').innerHTML =
    `Delete <strong>all ${employees.length} employee record(s)</strong>?<br>
     <span style="color:var(--text-muted);font-size:.82rem">This cannot be undone.</span>`;
  document.getElementById('confirmYes').onclick = () => {
    employees = [];
    pendingDeleteId = null;
    saveToStorage();
    refreshAll();
    closeConfirm();
    showToast('All records cleared', 'error');
  };
  openModal('confirmModal', 'confirmBackdrop');
}

/* ════════════════════════════════════════════════════════════
   MODAL HELPERS
   ════════════════════════════════════════════════════════════ */
function openModal(modalId, backdropId) {
  document.getElementById(modalId).classList.add('open');
  document.getElementById(backdropId).classList.add('open');
  document.body.style.overflow = 'hidden';
}

function closeModalById(modalId, backdropId) {
  document.getElementById(modalId).classList.remove('open');
  document.getElementById(backdropId).classList.remove('open');
  document.body.style.overflow = '';
}

/* Close on Escape */
document.addEventListener('keydown', e => {
  if (e.key === 'Escape') { closeModal(); closeConfirm(); }
});

/* ════════════════════════════════════════════════════════════
   LOCAL STORAGE
   ════════════════════════════════════════════════════════════ */
function saveToStorage() {
  try {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(employees));
  } catch (err) {
    showToast('Storage error — data may not be saved.', 'error');
  }
}

function loadFromStorage() {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (raw) {
      const parsed = JSON.parse(raw);
      // Re-compute payroll on load to ensure consistency
      employees = parsed.map(e => calculatePayroll({ ...e }));
    }
  } catch (err) {
    employees = [];
  }
}

/* ════════════════════════════════════════════════════════════
   UTILITIES
   ════════════════════════════════════════════════════════════ */

/** Check duplicate ID (case-insensitive) */
function isDuplicateId(id) {
  return employees.some(e => e.empId === id.toUpperCase());
}

/** Format number with 2 decimal places & thousand separators */
function fmt(n) {
  return Number(n).toLocaleString('en-IN', { minimumFractionDigits: 2, maximumFractionDigits: 2 });
}

/** Format large numbers with K/L suffix for dashboard */
function fmtK(n) {
  if (n >= 100000) return (n / 100000).toFixed(2) + ' L';
  if (n >= 1000)   return (n / 1000).toFixed(1) + 'K';
  return fmt(n);
}

/** Escape HTML to prevent XSS */
function esc(str) {
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

/** Show a brief toast message */
let _toastTimer = null;
function showToast(msg, type = '') {
  const toast = document.getElementById('toast');
  toast.textContent = msg;
  toast.className = `toast show ${type}`;
  clearTimeout(_toastTimer);
  _toastTimer = setTimeout(() => {
    toast.className = 'toast';
  }, 3000);
}

/** Refresh everything after any data change */
function refreshAll() {
  renderDashboard();
  renderTable();
  document.getElementById('storageBadge').textContent =
    `${employees.length} record${employees.length===1?'':'s'} stored`;
}
