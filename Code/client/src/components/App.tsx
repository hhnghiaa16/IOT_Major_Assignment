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
import { useAuth } from '../hooks/useAuth';

const App: React.FC = () => {
  const [currentPage, setCurrentPage] = useState('MyDevices');
  const [pageKey, setPageKey] = useState<number>(0);
  const { isLoggedIn, login, logout } = useAuth();

  // Force remount when page changes
  useEffect(() => {
    setPageKey((prev) => prev + 1);
  }, [currentPage]);

  if (!isLoggedIn) {
    return (
      <div className="app">
        <Header currentPage={currentPage} onNavigate={setCurrentPage} />
        <div className="container">
          <Login onLogin={login} />
        </div>
      </div>
    );
  }

  return (
    <div className="app">
      <Header currentPage={currentPage} onNavigate={setCurrentPage} onLogout={logout} />

      <div className="container">
        {currentPage === 'MyDevices' ? (
          <MyDevices key={`my-devices-${pageKey}`} onClose={() => {}} />
        ) : currentPage === 'Dashboard' ? (
          <Dashboard key={`dashboard-${pageKey}`} />
        ) : null}
      </div>
    </div>
  );
};

export default App;
