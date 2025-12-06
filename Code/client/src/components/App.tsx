/**
 * App Component
 * Main application component with routing logic
 */

import React, { useState, useEffect } from 'react';
import '../styles/App.css';
import Header from './Header';
import Dashboard from './dashboard/Dashboard';
import Login from './Login';
import MyDevices from './MyDevices';
import ChatPage from './ChatPage';
import ProtectedRoute from './ProtectedRoute';
import { useAuth } from '../hooks/useAuth';

const App: React.FC = () => {
  const [currentPage, setCurrentPage] = useState('Dashboard');
  const [pageKey, setPageKey] = useState<number>(0);
  const { isLoggedIn, login, logout } = useAuth();

  // Force remount when page changes
  useEffect(() => {
    setPageKey((prev) => prev + 1);
  }, [currentPage]);

  const handleNavigate = (page: string) => {
    setCurrentPage(page);
  };

  if (!isLoggedIn) {
    return (
      <div className="app">
        <div className="container">
          <Login onLogin={(user) => login(user)} />
        </div>
      </div>
    );
  }

  return (
    <div className="app">
      <Header currentPage={currentPage} onNavigate={handleNavigate} onLogout={logout} />

      <div className="container">
        {currentPage === 'MyDevices' ? (
          <ProtectedRoute allowedRoles={['admin', 'viewer']}>
            <MyDevices key={`my-devices-${pageKey}`} onClose={() => { }} />
          </ProtectedRoute>
        ) : currentPage === 'Dashboard' ? (
          <ProtectedRoute allowedRoles={['admin', 'viewer']}>
            <Dashboard key={`dashboard-${pageKey}`} />
          </ProtectedRoute>
        ) : currentPage === 'Chat' ? (
          <ProtectedRoute allowedRoles={['admin', 'viewer']}>
            <ChatPage key={`chat-${pageKey}`} />
          </ProtectedRoute>
        ) : null}
      </div>
    </div>
  );
};

export default App;
