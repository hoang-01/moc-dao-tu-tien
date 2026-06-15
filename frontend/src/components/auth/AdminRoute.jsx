import { Navigate, Outlet } from 'react-router-dom'
import { useAuthStore } from '../../store/authStore.js'
import Spinner from '../ui/Spinner.jsx'

// Chỉ cho phép user có role = "admin" truy cập. Dùng lồng trong ProtectedRoute.
export default function AdminRoute() {
  const { user, isLoading } = useAuthStore()

  if (isLoading || !user) return <Spinner fullscreen />

  if (user.role !== 'admin') {
    return <Navigate to="/plants" replace />
  }

  return <Outlet />
}
