import { useState, useEffect } from 'react'
import { adminApi } from '../services/api.js'
import Navbar from '../components/ui/Navbar.jsx'
import Spinner from '../components/ui/Spinner.jsx'
import toast from 'react-hot-toast'

const TABS = [
  { key: 'overview', label: 'Tổng quan', icon: '📊' },
  { key: 'devices', label: 'Thiết bị', icon: '📟' },
  { key: 'plant-types', label: 'Loại cây', icon: '🌿' },
  { key: 'exp', label: 'Hệ số Tu Vi', icon: '⚙️' },
  { key: 'ranks', label: 'Cảnh Giới', icon: '🏯' },
]

// ─── Shared style helpers ──────────────────────────────────────────────────────
const card = {
  background: 'var(--bg-surface)',
  border: '1px solid var(--border)',
  borderRadius: 'var(--radius-lg)',
  boxShadow: 'var(--shadow-sm)',
  padding: 20,
}
const inputStyle = {
  padding: '7px 10px', border: '1px solid var(--border-strong)',
  borderRadius: 'var(--radius-sm)', fontSize: 13.5, fontFamily: 'inherit',
  background: 'var(--bg-surface)', color: 'var(--text-primary)', width: '100%',
}
const btnPrimary = {
  padding: '8px 14px', background: 'var(--accent)', color: 'white', border: 'none',
  borderRadius: 'var(--radius-md)', fontSize: 13.5, fontWeight: 500, cursor: 'pointer',
}
const btnGhost = {
  padding: '6px 12px', background: 'transparent', color: 'var(--text-secondary)',
  border: '1px solid var(--border-strong)', borderRadius: 'var(--radius-sm)',
  fontSize: 12.5, cursor: 'pointer',
}
const th = {
  textAlign: 'left', padding: '10px 12px', fontSize: 12, fontWeight: 600,
  color: 'var(--text-muted)', textTransform: 'uppercase', letterSpacing: 0.5,
  borderBottom: '1px solid var(--border)', whiteSpace: 'nowrap',
}
const td = { padding: '10px 12px', fontSize: 13.5, color: 'var(--text-primary)', borderBottom: '1px solid var(--border)' }

export default function AdminPage() {
  const [tab, setTab] = useState('overview')

  return (
    <div style={{ minHeight: '100vh', background: 'var(--bg-base)' }}>
      <Navbar />
      <div style={{ maxWidth: 1000, margin: '0 auto', padding: '32px 20px', animation: 'fadeUp 0.4s ease both' }}>
        <h1 style={{ fontSize: 28, color: 'var(--text-primary)', marginBottom: 4 }}>Quản trị hệ thống</h1>
        <p style={{ color: 'var(--text-muted)', fontSize: 14, marginBottom: 24 }}>
          Quản lý thiết bị IoT, loại cây và hệ số Gamification
        </p>

        {/* Tab nav */}
        <div style={{ display: 'flex', gap: 6, flexWrap: 'wrap', marginBottom: 24 }}>
          {TABS.map((t) => (
            <button
              key={t.key}
              onClick={() => setTab(t.key)}
              style={{
                padding: '8px 14px', fontSize: 13.5, cursor: 'pointer',
                border: '1px solid', borderRadius: 20,
                borderColor: tab === t.key ? 'var(--accent)' : 'var(--border)',
                background: tab === t.key ? 'var(--green-50)' : 'transparent',
                color: tab === t.key ? 'var(--accent)' : 'var(--text-muted)',
                fontWeight: tab === t.key ? 600 : 400, transition: 'all 0.15s',
              }}
            >
              <span style={{ marginRight: 6 }}>{t.icon}</span>{t.label}
            </button>
          ))}
        </div>

        {tab === 'overview' && <OverviewTab />}
        {tab === 'devices' && <DevicesTab />}
        {tab === 'plant-types' && <PlantTypesTab />}
        {tab === 'exp' && <ExpConfigTab />}
        {tab === 'ranks' && <RankConfigTab />}
      </div>
    </div>
  )
}

// ═══ Tổng quan ════════════════════════════════════════════════════════════════
function OverviewTab() {
  const [stats, setStats] = useState(null)
  const [loading, setLoading] = useState(true)

  useEffect(() => {
    adminApi.getDashboard()
      .then(({ data }) => setStats(data))
      .catch(() => toast.error('Không thể tải thống kê'))
      .finally(() => setLoading(false))
  }, [])

  if (loading) return <Spinner />
  if (!stats) return null

  const cards = [
    { label: 'Người dùng', value: stats.total_users, icon: '👤' },
    { label: 'Thiết bị', value: stats.total_devices, icon: '📟' },
    { label: 'Online', value: stats.devices_online, icon: '🟢' },
    { label: 'Offline', value: stats.devices_offline, icon: '🔴' },
    { label: 'Cây đã liên kết', value: stats.total_plants_paired, icon: '🪴' },
  ]
  const maxDaily = Math.max(1, ...(stats.new_users_daily || []).map((d) => d.count))

  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: 20 }}>
      {/* Stat cards */}
      <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(150px, 1fr))', gap: 14 }}>
        {cards.map((c) => (
          <div key={c.label} style={{ ...card, padding: 18 }}>
            <div style={{ fontSize: 22, marginBottom: 6 }}>{c.icon}</div>
            <div style={{ fontSize: 26, fontFamily: 'var(--font-display)', color: 'var(--text-secondary)' }}>
              {Number(c.value || 0).toLocaleString('vi-VN')}
            </div>
            <div style={{ fontSize: 12.5, color: 'var(--text-muted)' }}>{c.label}</div>
          </div>
        ))}
      </div>

      {/* Rank distribution */}
      <div style={card}>
        <h3 style={{ fontSize: 15, fontWeight: 500, marginBottom: 16, color: 'var(--text-primary)' }}>
          Phân bố Cảnh Giới
        </h3>
        {(stats.rank_distribution || []).length === 0 ? (
          <p style={{ fontSize: 13, color: 'var(--text-muted)' }}>Chưa có dữ liệu</p>
        ) : (
          <div style={{ display: 'flex', flexDirection: 'column', gap: 10 }}>
            {stats.rank_distribution.map((r) => {
              const total = stats.total_plants_paired || 1
              const pct = Math.round((r.count / total) * 100)
              return (
                <div key={r.rank_name} style={{ display: 'flex', alignItems: 'center', gap: 12 }}>
                  <span style={{ width: 110, fontSize: 13, color: 'var(--text-secondary)', flexShrink: 0 }}>{r.rank_name}</span>
                  <div style={{ flex: 1, height: 10, background: 'var(--bg-soft)', borderRadius: 6, overflow: 'hidden' }}>
                    <div style={{ width: `${pct}%`, height: '100%', background: 'var(--green-400)' }} />
                  </div>
                  <span style={{ width: 36, textAlign: 'right', fontSize: 12.5, color: 'var(--text-muted)' }}>{r.count}</span>
                </div>
              )
            })}
          </div>
        )}
      </div>

      {/* New users daily */}
      <div style={card}>
        <h3 style={{ fontSize: 15, fontWeight: 500, marginBottom: 16, color: 'var(--text-primary)' }}>
          Người dùng mới (7 ngày)
        </h3>
        <div style={{ display: 'flex', alignItems: 'flex-end', gap: 8, height: 120 }}>
          {(stats.new_users_daily || []).map((d) => (
            <div key={d.date} style={{ flex: 1, display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 6 }}>
              <span style={{ fontSize: 11, color: 'var(--text-muted)' }}>{d.count}</span>
              <div style={{
                width: '100%', maxWidth: 40,
                height: `${(d.count / maxDaily) * 80 + 4}px`,
                background: 'var(--green-300)', borderRadius: '4px 4px 0 0',
              }} />
              <span style={{ fontSize: 10.5, color: 'var(--text-muted)' }}>
                {new Date(d.date).toLocaleDateString('vi-VN', { day: '2-digit', month: '2-digit' })}
              </span>
            </div>
          ))}
        </div>
      </div>
    </div>
  )
}

// ═══ Thiết bị ═════════════════════════════════════════════════════════════════
function DevicesTab() {
  const [devices, setDevices] = useState([])
  const [loading, setLoading] = useState(true)
  const [creating, setCreating] = useState(false)
  const [newDevice, setNewDevice] = useState(null) // hiển thị verify_code 1 lần

  const load = async () => {
    try {
      const { data } = await adminApi.listDevices()
      setDevices(data)
    } catch {
      toast.error('Không thể tải danh sách thiết bị')
    } finally {
      setLoading(false)
    }
  }
  useEffect(() => { load() }, [])

  const handleCreate = async () => {
    setCreating(true)
    try {
      const { data } = await adminApi.createDevice()
      setNewDevice(data)
      toast.success('Đã tạo thiết bị mới')
      load()
    } catch {
      toast.error('Tạo thiết bị thất bại')
    } finally {
      setCreating(false)
    }
  }

  const toggleActive = async (d) => {
    try {
      await adminApi.updateDevice(d.id, !d.is_active)
      toast.success(d.is_active ? 'Đã vô hiệu hóa' : 'Đã kích hoạt')
      load()
    } catch {
      toast.error('Cập nhật thất bại')
    }
  }

  if (loading) return <Spinner />

  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: 16 }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
        <span style={{ fontSize: 13.5, color: 'var(--text-muted)' }}>{devices.length} thiết bị</span>
        <button style={btnPrimary} disabled={creating} onClick={handleCreate}>
          {creating ? 'Đang tạo…' : '+ Cấp thiết bị mới'}
        </button>
      </div>

      {/* Verify code reveal (chỉ hiện 1 lần) */}
      {newDevice && (
        <div style={{
          ...card, background: 'var(--green-50)', border: '1px solid var(--green-300)',
          display: 'flex', justifyContent: 'space-between', alignItems: 'center', gap: 12, flexWrap: 'wrap',
        }}>
          <div>
            <p style={{ fontSize: 13, color: 'var(--text-secondary)', marginBottom: 6 }}>
              ⚠️ Verify Code chỉ hiển thị <b>một lần duy nhất</b> — hãy in lên thiết bị ngay:
            </p>
            <div style={{ display: 'flex', gap: 20, fontSize: 14 }}>
              <span>Plant Code: <code style={{ fontWeight: 600 }}>{newDevice.plant_code}</code></span>
              <span>Verify Code: <code style={{ fontWeight: 600, color: 'var(--accent)' }}>{newDevice.verify_code}</code></span>
            </div>
          </div>
          <button style={btnGhost} onClick={() => setNewDevice(null)}>Đã lưu, đóng</button>
        </div>
      )}

      <div style={{ ...card, padding: 0, overflowX: 'auto' }}>
        <table style={{ width: '100%', borderCollapse: 'collapse', minWidth: 640 }}>
          <thead>
            <tr>
              <th style={th}>Plant Code</th>
              <th style={th}>Trạng thái</th>
              <th style={th}>Liên kết</th>
              <th style={th}>Online lần cuối</th>
              <th style={th}></th>
            </tr>
          </thead>
          <tbody>
            {devices.length === 0 ? (
              <tr><td style={{ ...td, textAlign: 'center', color: 'var(--text-muted)' }} colSpan={5}>Chưa có thiết bị nào</td></tr>
            ) : devices.map((d) => (
              <tr key={d.id}>
                <td style={td}><code>{d.plant_code}</code></td>
                <td style={td}>
                  <span style={{
                    fontSize: 12, padding: '2px 9px', borderRadius: 12,
                    background: d.is_active ? 'var(--green-100)' : '#fde8e6',
                    color: d.is_active ? 'var(--green-700)' : '#c0392b',
                  }}>
                    {d.is_active ? 'Đang bật' : 'Vô hiệu'}
                  </span>
                </td>
                <td style={td}>
                  {d.is_paired
                    ? <span style={{ fontSize: 12.5 }}>{d.paired_plant_name}<br /><span style={{ color: 'var(--text-muted)', fontSize: 11.5 }}>{d.paired_user_email}</span></span>
                    : <span style={{ color: 'var(--text-muted)', fontSize: 12.5 }}>Chưa liên kết</span>}
                </td>
                <td style={{ ...td, fontSize: 12.5, color: 'var(--text-muted)' }}>
                  {d.last_seen_at ? new Date(d.last_seen_at).toLocaleString('vi-VN') : '—'}
                </td>
                <td style={td}>
                  <button style={btnGhost} onClick={() => toggleActive(d)}>
                    {d.is_active ? 'Vô hiệu hóa' : 'Kích hoạt'}
                  </button>
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  )
}

// ═══ Loại cây ═════════════════════════════════════════════════════════════════
const EMPTY_TYPE = {
  name: '', description: '',
  soil_moisture_min: 40, soil_moisture_max: 70,
  light_min: 1000, light_max: 10000,
  temperature_min: 20, temperature_max: 30,
  humidity_min: 50, humidity_max: 80,
}
const THRESHOLD_FIELDS = [
  ['soil_moisture_min', 'soil_moisture_max', 'Độ ẩm đất (%)'],
  ['light_min', 'light_max', 'Ánh sáng (lux)'],
  ['temperature_min', 'temperature_max', 'Nhiệt độ (°C)'],
  ['humidity_min', 'humidity_max', 'Độ ẩm KK (%)'],
]

function PlantTypesTab() {
  const [types, setTypes] = useState([])
  const [loading, setLoading] = useState(true)
  const [editing, setEditing] = useState(null) // null | {id?, ...fields}

  const load = async () => {
    try {
      const { data } = await adminApi.listPlantTypes()
      setTypes(data)
    } catch {
      toast.error('Không thể tải loại cây')
    } finally {
      setLoading(false)
    }
  }
  useEffect(() => { load() }, [])

  const save = async () => {
    if (!editing.name?.trim()) return toast.error('Vui lòng nhập tên loại cây')
    try {
      const payload = { ...editing }
      delete payload.id
      delete payload.created_at
      delete payload.updated_at
      if (editing.id) await adminApi.updatePlantType(editing.id, payload)
      else await adminApi.createPlantType(payload)
      toast.success('Đã lưu loại cây')
      setEditing(null)
      load()
    } catch (e) {
      toast.error(e.response?.data?.detail || 'Lưu thất bại')
    }
  }

  const remove = async (t) => {
    if (!window.confirm(`Xóa loại cây "${t.name}"?`)) return
    try {
      await adminApi.deletePlantType(t.id)
      toast.success('Đã xóa')
      load()
    } catch (e) {
      toast.error(e.response?.data?.detail || 'Xóa thất bại')
    }
  }

  if (loading) return <Spinner />

  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: 16 }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
        <span style={{ fontSize: 13.5, color: 'var(--text-muted)' }}>{types.length} loại cây</span>
        <button style={btnPrimary} onClick={() => setEditing({ ...EMPTY_TYPE })}>+ Thêm loại cây</button>
      </div>

      {/* Edit form */}
      {editing && (
        <div style={card}>
          <h3 style={{ fontSize: 15, fontWeight: 500, marginBottom: 16 }}>
            {editing.id ? 'Sửa loại cây' : 'Thêm loại cây mới'}
          </h3>
          <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 12, marginBottom: 12 }}>
            <label style={{ fontSize: 12.5, color: 'var(--text-muted)' }}>
              Tên loại cây
              <input style={{ ...inputStyle, marginTop: 4 }} value={editing.name}
                onChange={(e) => setEditing({ ...editing, name: e.target.value })} />
            </label>
            <label style={{ fontSize: 12.5, color: 'var(--text-muted)' }}>
              Mô tả
              <input style={{ ...inputStyle, marginTop: 4 }} value={editing.description || ''}
                onChange={(e) => setEditing({ ...editing, description: e.target.value })} />
            </label>
          </div>
          <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(200px, 1fr))', gap: 12, marginBottom: 16 }}>
            {THRESHOLD_FIELDS.map(([minK, maxK, label]) => (
              <div key={minK}>
                <span style={{ fontSize: 12.5, color: 'var(--text-muted)' }}>{label}</span>
                <div style={{ display: 'flex', gap: 8, marginTop: 4, alignItems: 'center' }}>
                  <input type="number" style={inputStyle} value={editing[minK]}
                    onChange={(e) => setEditing({ ...editing, [minK]: parseFloat(e.target.value) })} />
                  <span style={{ color: 'var(--text-muted)' }}>–</span>
                  <input type="number" style={inputStyle} value={editing[maxK]}
                    onChange={(e) => setEditing({ ...editing, [maxK]: parseFloat(e.target.value) })} />
                </div>
              </div>
            ))}
          </div>
          <div style={{ display: 'flex', gap: 10 }}>
            <button style={btnPrimary} onClick={save}>Lưu</button>
            <button style={btnGhost} onClick={() => setEditing(null)}>Hủy</button>
          </div>
        </div>
      )}

      <div style={{ ...card, padding: 0, overflowX: 'auto' }}>
        <table style={{ width: '100%', borderCollapse: 'collapse', minWidth: 640 }}>
          <thead>
            <tr>
              <th style={th}>Tên</th>
              <th style={th}>Độ ẩm đất</th>
              <th style={th}>Ánh sáng</th>
              <th style={th}>Nhiệt độ</th>
              <th style={th}>Độ ẩm KK</th>
              <th style={th}></th>
            </tr>
          </thead>
          <tbody>
            {types.length === 0 ? (
              <tr><td style={{ ...td, textAlign: 'center', color: 'var(--text-muted)' }} colSpan={6}>Chưa có loại cây nào</td></tr>
            ) : types.map((t) => (
              <tr key={t.id}>
                <td style={td}><b>{t.name}</b>{t.description && <div style={{ fontSize: 11.5, color: 'var(--text-muted)' }}>{t.description}</div>}</td>
                <td style={td}>{t.soil_moisture_min}–{t.soil_moisture_max}</td>
                <td style={td}>{t.light_min}–{t.light_max}</td>
                <td style={td}>{t.temperature_min}–{t.temperature_max}</td>
                <td style={td}>{t.humidity_min}–{t.humidity_max}</td>
                <td style={{ ...td, whiteSpace: 'nowrap' }}>
                  <button style={{ ...btnGhost, marginRight: 6 }} onClick={() => setEditing({ ...t })}>Sửa</button>
                  <button style={{ ...btnGhost, color: '#c0392b', borderColor: '#e8b0aa' }} onClick={() => remove(t)}>Xóa</button>
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  )
}

// ═══ Hệ số Tu Vi (EXP) ════════════════════════════════════════════════════════
function ExpConfigTab() {
  const [configs, setConfigs] = useState([])
  const [loading, setLoading] = useState(true)
  const [saving, setSaving] = useState(false)

  const load = async () => {
    try {
      const { data } = await adminApi.getExpConfig()
      setConfigs(data)
    } catch {
      toast.error('Không thể tải hệ số Tu Vi')
    } finally {
      setLoading(false)
    }
  }
  useEffect(() => { load() }, [])

  const save = async () => {
    setSaving(true)
    try {
      await adminApi.updateExpConfig(configs.map((c) => ({
        quality_level: c.quality_level,
        exp_delta: parseFloat(c.exp_delta),
        description: c.description || null,
      })))
      toast.success('Đã cập nhật hệ số Tu Vi')
      load()
    } catch {
      toast.error('Lưu thất bại')
    } finally {
      setSaving(false)
    }
  }

  if (loading) return <Spinner />

  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: 16 }}>
      <p style={{ fontSize: 13.5, color: 'var(--text-muted)' }}>
        Mỗi mức chất lượng cảm biến tương ứng với lượng Tu Vi cộng/trừ mỗi chu kỳ.
      </p>
      <div style={{ ...card, padding: 0, overflowX: 'auto' }}>
        <table style={{ width: '100%', borderCollapse: 'collapse', minWidth: 520 }}>
          <thead>
            <tr>
              <th style={th}>Mức chất lượng</th>
              <th style={th}>EXP delta</th>
              <th style={th}>Mô tả</th>
            </tr>
          </thead>
          <tbody>
            {configs.map((c, i) => (
              <tr key={c.id || c.quality_level}>
                <td style={td}><code>{c.quality_level}</code></td>
                <td style={td}>
                  <input type="number" step="0.1" style={{ ...inputStyle, width: 110 }} value={c.exp_delta}
                    onChange={(e) => {
                      const next = [...configs]; next[i] = { ...c, exp_delta: e.target.value }; setConfigs(next)
                    }} />
                </td>
                <td style={td}>
                  <input style={inputStyle} value={c.description || ''}
                    onChange={(e) => {
                      const next = [...configs]; next[i] = { ...c, description: e.target.value }; setConfigs(next)
                    }} />
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
      <div><button style={btnPrimary} disabled={saving} onClick={save}>{saving ? 'Đang lưu…' : 'Lưu thay đổi'}</button></div>
    </div>
  )
}

// ═══ Cảnh Giới (Rank) ═════════════════════════════════════════════════════════
function RankConfigTab() {
  const [ranks, setRanks] = useState([])
  const [loading, setLoading] = useState(true)
  const [saving, setSaving] = useState(false)

  const load = async () => {
    try {
      const { data } = await adminApi.getRankConfig()
      setRanks(data)
    } catch {
      toast.error('Không thể tải Cảnh Giới')
    } finally {
      setLoading(false)
    }
  }
  useEffect(() => { load() }, [])

  const save = async () => {
    setSaving(true)
    try {
      await adminApi.updateRankConfig(ranks.map((r) => ({
        order: parseInt(r.order, 10),
        name: r.name,
        min_exp: parseFloat(r.min_exp),
      })))
      toast.success('Đã cập nhật Cảnh Giới')
      load()
    } catch {
      toast.error('Lưu thất bại')
    } finally {
      setSaving(false)
    }
  }

  const addRank = () => {
    const nextOrder = ranks.length ? Math.max(...ranks.map((r) => r.order)) + 1 : 1
    setRanks([...ranks, { order: nextOrder, name: '', min_exp: 0 }])
  }

  if (loading) return <Spinner />

  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: 16 }}>
      <p style={{ fontSize: 13.5, color: 'var(--text-muted)' }}>
        Các mốc Tu Vi để đột phá lên Cảnh Giới cao hơn (sắp xếp theo thứ tự tăng dần).
      </p>
      <div style={{ ...card, padding: 0, overflowX: 'auto' }}>
        <table style={{ width: '100%', borderCollapse: 'collapse', minWidth: 480 }}>
          <thead>
            <tr>
              <th style={th}>Thứ tự</th>
              <th style={th}>Tên Cảnh Giới</th>
              <th style={th}>Tu Vi tối thiểu</th>
            </tr>
          </thead>
          <tbody>
            {ranks.map((r, i) => (
              <tr key={r.id || i}>
                <td style={td}>
                  <input type="number" style={{ ...inputStyle, width: 70 }} value={r.order}
                    onChange={(e) => { const n = [...ranks]; n[i] = { ...r, order: e.target.value }; setRanks(n) }} />
                </td>
                <td style={td}>
                  <input style={inputStyle} value={r.name}
                    onChange={(e) => { const n = [...ranks]; n[i] = { ...r, name: e.target.value }; setRanks(n) }} />
                </td>
                <td style={td}>
                  <input type="number" style={{ ...inputStyle, width: 130 }} value={r.min_exp}
                    onChange={(e) => { const n = [...ranks]; n[i] = { ...r, min_exp: e.target.value }; setRanks(n) }} />
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
      <div style={{ display: 'flex', gap: 10 }}>
        <button style={btnPrimary} disabled={saving} onClick={save}>{saving ? 'Đang lưu…' : 'Lưu thay đổi'}</button>
        <button style={btnGhost} onClick={addRank}>+ Thêm Cảnh Giới</button>
      </div>
    </div>
  )
}
