import { useState, useEffect } from 'react'
import { leaderboardApi } from '../services/api.js'
import Navbar from '../components/ui/Navbar.jsx'
import Spinner from '../components/ui/Spinner.jsx'
import { useAuthStore } from '../store/authStore.js'
import toast from 'react-hot-toast'

// Huy chương cho top 3
const MEDALS = { 1: '🥇', 2: '🥈', 3: '🥉' }

export default function LeaderboardPage() {
  const { user } = useAuthStore()
  const [entries, setEntries] = useState([])
  const [totalCount, setTotalCount] = useState(0)
  const [loading, setLoading] = useState(true)

  useEffect(() => {
    const fetch = async () => {
      try {
        const { data } = await leaderboardApi.getLeaderboard(50)
        setEntries(data.entries || [])
        setTotalCount(data.total_count || 0)
      } catch {
        toast.error('Không thể tải bảng xếp hạng')
      } finally {
        setLoading(false)
      }
    }
    fetch()
  }, [])

  if (loading) return (
    <div style={{ minHeight: '100vh', background: 'var(--bg-base)' }}>
      <Navbar />
      <div style={{ paddingTop: 80 }}><Spinner fullscreen /></div>
    </div>
  )

  return (
    <div style={{ minHeight: '100vh', background: 'var(--bg-base)' }}>
      <Navbar />

      <div style={{ maxWidth: 760, margin: '0 auto', padding: '32px 20px', animation: 'fadeUp 0.4s ease both' }}>

        <div style={{ marginBottom: 24, textAlign: 'center' }}>
          <div style={{ fontSize: 40, marginBottom: 8 }}>🏆</div>
          <h1 style={{ fontSize: 28, color: 'var(--text-primary)', marginBottom: 4 }}>
            Bảng Xếp Hạng Tu Vi
          </h1>
          <p style={{ color: 'var(--text-muted)', fontSize: 14 }}>
            {totalCount} đạo hữu đang cùng tu luyện
          </p>
        </div>

        {entries.length === 0 ? (
          <div style={{
            textAlign: 'center', padding: '64px 20px', background: 'var(--bg-surface)',
            border: '1px dashed var(--border-strong)', borderRadius: 'var(--radius-lg)',
            color: 'var(--text-muted)',
          }}>
            <div style={{ fontSize: 48, marginBottom: 16 }}>🌱</div>
            <p style={{ fontSize: 14 }}>Chưa có đạo hữu nào trên bảng xếp hạng</p>
          </div>
        ) : (
          <div style={{
            background: 'var(--bg-surface)',
            border: '1px solid var(--border)',
            borderRadius: 'var(--radius-lg)',
            boxShadow: 'var(--shadow-sm)',
            overflow: 'hidden',
          }}>
            {entries.map((e) => {
              const isMe = user && e.owner_display_name === user.display_name
              const top3 = e.rank <= 3
              return (
                <div
                  key={e.plant_id}
                  style={{
                    display: 'flex', alignItems: 'center', gap: 14,
                    padding: '14px 18px',
                    borderBottom: '1px solid var(--border)',
                    background: isMe ? 'var(--green-50)' : 'transparent',
                  }}
                >
                  {/* Rank number / medal */}
                  <div style={{
                    width: 40, textAlign: 'center', flexShrink: 0,
                    fontSize: top3 ? 24 : 16,
                    fontWeight: 600,
                    color: top3 ? 'inherit' : 'var(--text-muted)',
                    fontFamily: top3 ? 'inherit' : 'var(--font-display)',
                  }}>
                    {MEDALS[e.rank] || e.rank}
                  </div>

                  {/* Plant avatar */}
                  <div style={{
                    width: 42, height: 42, flexShrink: 0,
                    background: 'var(--green-50)',
                    border: '1.5px solid var(--green-200)',
                    borderRadius: '50%',
                    display: 'flex', alignItems: 'center', justifyContent: 'center',
                    fontSize: 20,
                  }}>
                    🪴
                  </div>

                  {/* Name + owner */}
                  <div style={{ flex: 1, minWidth: 0 }}>
                    <div style={{
                      fontSize: 15, fontWeight: 500, color: 'var(--text-primary)',
                      overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap',
                    }}>
                      {e.plant_name || 'Cây chưa đặt tên'}
                      {isMe && (
                        <span style={{
                          marginLeft: 8, fontSize: 11, color: 'var(--accent)',
                          background: 'var(--green-100)', padding: '1px 7px', borderRadius: 10,
                        }}>
                          Bạn
                        </span>
                      )}
                    </div>
                    <div style={{
                      fontSize: 12.5, color: 'var(--text-muted)',
                      overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap',
                    }}>
                      {e.owner_display_name} · {e.rank_name}
                    </div>
                  </div>

                  {/* Tu Vi */}
                  <div style={{ textAlign: 'right', flexShrink: 0 }}>
                    <div style={{
                      fontSize: 17, fontFamily: 'var(--font-display)',
                      color: 'var(--text-secondary)', lineHeight: 1.1,
                    }}>
                      {Number(e.total_exp || 0).toLocaleString('vi-VN')}
                    </div>
                    <div style={{ fontSize: 10.5, color: 'var(--text-muted)', letterSpacing: 1, textTransform: 'uppercase' }}>
                      Tu Vi
                    </div>
                  </div>
                </div>
              )
            })}
          </div>
        )}
      </div>
    </div>
  )
}
